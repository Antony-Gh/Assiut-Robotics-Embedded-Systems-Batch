# 6. Topic 5 — Actuator Interfacing and Power Electronics Fundamentals

Embedded firmware does not exist in a vacuum; its ultimate purpose in robotics is to interact physically with the real world through **actuators**: spinning DC drive motors, steering servo linkages, positioning precision stepper stages, and firing solenoid valves.

However, moving from sensory inputs to mechanical actuation presents a severe electrical danger to the microcontroller. An STM32 GPIO pin operates in the low-power **signal domain** ($3.3\text{ V}$, milliampere currents). Electric motors operate in the **power domain** ($6\text{ V} - 48\text{ V}$, currents from hundreds of milliamperes to dozens of amperes, and violent inductive voltage spikes).

Connecting a motor directly to an STM32 pin will instantly destroy the microcontroller silicon.

This chapter bridges firmware algorithms and power electronics hardware. We analyze the electrical limits of STM32 GPIOs, dissect H-Bridge motor driver topologies (comparing the legacy L298N with modern MOSFET drivers like the TB6612FNG and DRV8833), implement $50\text{ Hz}$ hobby servo control, detail stepper motor microstepping with trapezoidal motion profiling, and formulate strict power isolation and flyback protection rules.

![Figure 5.1 — Actuator driver topologies, H-Bridge switching states, servo timing, and stepper signals](diagrams_adv/fig5_actuators_and_power.png)
*Figure 5.1 — (A) Full H-Bridge DC motor drive topology showing high-side and low-side switching paths, shoot-through hazard, and flyback protection diodes; (B) Servo 50 Hz PWM pulse width encoding ($1.0\text{ ms} - 2.0\text{ ms}$) and stepper driver STEP/DIR timing.*

---

## 6.1 The Signal Domain vs. The Power Domain

**Table 6.1 — Signal domain vs. power domain characteristics**

| Parameter | STM32 Microcontroller (Signal Domain) | Electric Motors & Actuators (Power Domain) |
|---|---|---|
| **Operating Voltage** | Strict $3.3\text{ V} \pm 5\%$ ($2.0\text{ V} - 3.6\text{ V}$) | $6\text{ V}, 12\text{ V}, 24\text{ V}, \text{or } 48\text{ V}$ battery rails |
| **Current Capability** | **$\le 8\text{ mA}$ recommended** ($25\text{ mA}$ absolute limit) | **$500\text{ mA} - 30+\text{ A}$** (Surges at stall) |
| **Output Type** | CMOS digital logic states (0V or 3.3V) | High-current inductive continuous/pulsed current |
| **Noise Tolerance** | Highly sensitive to EMI, ground bounce, and spikes | Massive generator of electromagnetic noise and back-EMF |

### 6.1.1 STM32 GPIO Current Limitations
According to the *STM32F103 and STM32F405 Electrical Characteristics* datasheets:
- **Maximum Output Current on a Single I/O Pin ($I_{IO}$)**: Strictly **$25\text{ mA}$**.
- **Recommended Continuous Output Current**: Strictly **$\le 8\text{ mA}$** to guarantee valid output logic levels ($V_{OL} \le 0.4\text{ V}$, $V_{OH} \ge V_{DD} - 0.4\text{ V}$).
- **Total Chip Current Sunk/Sourced Across ALL I/Os ($I_{DD} / I_{SS}$)**: Strictly **$150\text{ mA}$**.

If an engineer connects even a tiny 5V toy DC motor (which draws $\sim 200\text{ mA}$ running and $> 1.5\text{ A}$ at stall) to an STM32 pin, the internal CMOS output driver transistors will melt due to localized thermal breakdown within milliseconds.

---

### 6.1.2 Electrical Isolation and Common Ground Architecture
To safely control high-power actuators from low-voltage logic:
1. **Dedicated Power Supplies**: The microcontroller logic must be powered by a clean, regulated 3.3V supply (e.g., via a buck regulator or LDO). Motors must be powered directly from the raw battery pack.
2. **Common Ground (Star Grounding)**: The microcontroller GND and motor driver GND must connect at a **single physical point (Star Ground)** right at the battery terminal. Connecting motor ground returns through the microcontroller PCB ground plane will cause massive ground bounce, resetting the CPU.
3. **Optocouplers / Digital Isolators (Industrial Standard)**: For high-voltage or high-current systems (> 24V or > 10A), insert high-speed optocouplers (such as the PC817 or 6N137) or capacitive digital isolators (ADuM1201) to achieve complete galvanic isolation between STM32 pins and motor gates.

---

## 6.2 DC Motor Control Using H-Bridges

A single switch or transistor can turn a motor ON and OFF in one direction. However, reversing a DC motor requires reversing the polarity of the voltage applied across its two terminals. This is accomplished using an **H-Bridge**.

### 6.2.1 Operating Principle of the H-Bridge
An H-Bridge consists of four electronic switches arranged in the shape of an "H" around a central DC motor:

```text
            +VMOTOR (Power Rail)
              │           │
             [Q1]        [Q2]   (High-Side Switches: P-FET or N-FET)
              │           │
              ├─────[M]───┤
              │   (Motor) │
             [Q3]        [Q4]   (Low-Side Switches: N-FET)
              │           │
            ═════════════════   (Power Ground)
```

**Table 6.2 — H-Bridge switching truth table**

| Q1 (High Left) | Q2 (High Right) | Q3 (Low Left) | Q4 (Low Right) | Motor Terminals State | Motor Behavioral Response |
|---|---|---|---|---|---|
| **ON** | OFF | OFF | **ON** | Left = $+V_{\text{MOT}}$, Right = GND | **Forward Rotation** |
| OFF | **ON** | **ON** | OFF | Left = GND, Right = $+V_{\text{MOT}}$ | **Reverse Rotation** |
| OFF | OFF | **ON** | **ON** | Left = GND, Right = GND | **Brake Mode (Dynamic Braking)** |
| OFF | OFF | OFF | OFF | Floating / High-Impedance | **Coast Mode (Freewheel to a stop)** |
| **ON** | OFF | **ON** | OFF | **SHORT CIRCUIT: $V_{\text{MOT}}$ directly to GND!** | **SHOOT-THROUGH: DESTROYS FETS!** |

> [!CAUTION]
> **The Shoot-Through Hazard**: If Q1 and Q3 (or Q2 and Q4) turn ON simultaneously, raw battery voltage shorts directly to ground through the transistors. The resulting hundred-ampere inrush current destroys the H-Bridge in microseconds. Motor driver ICs (TB6612, DRV8833) incorporate internal hardware logic to prevent shoot-through.

---

### 6.2.2 Motor Driver Comparison: L298N vs. TB6612FNG vs. DRV8833

**Table 6.3 — Detailed motor driver hardware comparison**

| Parameter | L298N (Legacy Bipolar) | TB6612FNG (Dual MOSFET) | DRV8833 (Low-Voltage Dual MOSFET) |
|---|---|---|---|
| **Output Transistor Type** | Bipolar Junction Transistors (BJT) | Power MOSFETs ($R_{DS(\text{on})} \approx 0.5\ \Omega$) | Power MOSFETs ($R_{DS(\text{on})} \approx 0.36\ \Omega$) |
| **Internal Voltage Drop ($V_{\text{drop}}$)**| **Massive: $1.8\text{ V} - 3.2\text{ V}$**! | **Negligible: $0.2\text{ V} - 0.4\text{ V}$** | **Negligible: $0.15\text{ V} - 0.3\text{ V}$** |
| **Power Dissipation & Heat** | Extreme; requires massive aluminum heatsink | Runs cool; tiny $5 \times 5\text{ mm}$ surface mount | Runs cool; minimal thermal rise |
| **Motor Supply Voltage ($V_M$)** | $4.5\text{ V} - 46\text{ V}$ | $4.5\text{ V} - 15\text{ V}$ | **$2.7\text{ V} - 10.8\text{ V}$** (Ideal for 1S/2S LiPo) |
| **Continuous Current per Channel**| $2.0\text{ A}$ (With heatsink) | $1.2\text{ A}$ continuous ($3.2\text{ A}$ peak) | $1.5\text{ A}$ continuous ($2.0\text{ A}$ peak) |
| **Logic Voltage ($V_{CC}$)** | $5.0\text{ V}$ only (Requires level shifting) | **$2.7\text{ V} - 5.5\text{ V}$ (Native 3.3V STM32 compatible)**| **$2.7\text{ V} - 5.5\text{ V}$ (Native 3.3V STM32 compatible)**|
| **Verdict for Modern Robotics** | **OBSOLETE**. Avoid in modern designs. | **RECOMMENDED** for 12V mobile robots. | **RECOMMENDED** for low-voltage/micro-robots. |

---

### 6.2.3 Connecting and Programming the TB6612FNG with STM32 HAL

To drive a motor using the TB6612FNG:
- **`PWMA`**: Connected to a Timer PWM output channel (e.g., `TIM1_CH1` on PA8) to modulate speed ($0\% - 100\%$).
- **`AIN1` and `AIN2`**: Connected to standard GPIO push-pull output pins (e.g., `PB12` and `PB13`) to establish direction.
- **`STBY` (Standby)**: Connected to a GPIO pin (e.g., `PB14`); must be pulled **HIGH** to enable the driver.

#### Why $20\text{ kHz}$ PWM is the Industry Standard
Novice students often configure timer PWM frequencies at $1\text{ kHz}$ or $2\text{ kHz}$.
- *The Problem*: The human ear can hear frequencies between $20\text{ Hz}$ and $20\text{ kHz}$. At $1\text{ kHz}$, the motor windings vibrate mechanically like an audio speaker, emitting an infuriating, loud, high-pitched squeal.
- *The Solution*: Configure the timer PWM frequency to **$20\text{ kHz}$ (ultrasonic)**. The motor operates in complete silence, and motor current ripple is smoothed significantly by winding inductance.

#### Timer Math for $20\text{ kHz}$ PWM on STM32F103 ($72\text{ MHz}$ Clock)
We require:

$$f_{\text{PWM}} = \frac{f_{\text{TIM}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)} = 20{,}000\text{ Hz}$$

Setting $\text{PSC} = 0$ (no prescaler, timer counts at $72\text{ MHz}$):

$$\text{ARR} + 1 = \frac{72{,}000{,}000}{20{,}000} = 3600 \implies \mathbf{\text{ARR} = 3599}$$

Setting duty cycle:
- $0\%$ Speed: $\text{CCR} = 0$
- $50\%$ Speed: $\text{CCR} = 1800$
- $100\%$ Full Speed: $\text{CCR} = 3600$

**Listing 6.1 — Verified TB6612FNG motor driver module**

```c
#include "main.h"

extern TIM_HandleTypeDef htim1;

#define MOTOR_DIR_PORT    GPIOB
#define MOTOR_AIN1_PIN    GPIO_PIN_12
#define MOTOR_AIN2_PIN    GPIO_PIN_13
#define MOTOR_STBY_PIN    GPIO_PIN_14

void Motor_Init(void)
{
    /* Start PWM on TIM1 Channel 1 (PA8) */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    
    /* Take TB6612 out of standby */
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_STBY_PIN, GPIO_PIN_SET);
    
    /* Initially stopped */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
}

/**
 * @brief Sets motor velocity with signed speed input.
 * @param speed Signed value between -3600 (full reverse) and +3600 (full forward).
 */
void Motor_SetSpeed(int16_t speed)
{
    if (speed > 3600)  speed = 3600;
    if (speed < -3600) speed = -3600;

    if (speed > 0) {
        /* Forward Rotation: AIN1 = HIGH, AIN2 = LOW */
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_AIN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_AIN2_PIN, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)speed);
    } 
    else if (speed < 0) {
        /* Reverse Rotation: AIN1 = LOW, AIN2 = HIGH */
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_AIN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_AIN2_PIN, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)(-speed));
    } 
    else {
        /* Active Brake: AIN1 = HIGH, AIN2 = HIGH, PWM = 0 */
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_AIN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_AIN2_PIN, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    }
}
```

---

## 6.3 Hobby Servo Motor Interfacing

A hobby servo (e.g., TowerPro SG90, MG996R) is a closed-loop actuator containing an internal DC motor, a multi-stage reduction gearbox, an output shaft potentiometer, and an internal analog/digital feedback amplifier.

### 6.3.1 The $50\text{ Hz}$ Pulse-Width Position Protocol
Unlike a DC motor, where PWM *duty cycle* controls speed, a servo's angular position is governed strictly by the **absolute high-time pulse width ($T_{\text{high}}$)** within a fixed **$50\text{ Hz}$ ($20\text{ ms}$)** repeating period.

```text
 50 Hz PWM Frame (Period = 20 ms / 20,000 µs):
 ┌──────┐
 │1.0 ms│ 0 Degrees (Full Left)
 └──────┴──────────────────────────────────────────────── 19.0 ms LOW ──
 
 ┌─────────┐
 │ 1.5 ms  │ 90 Degrees (Neutral / Center Position)
 └─────────┴────────────────────────────────────────────── 18.5 ms LOW ──
 
 ┌────────────┐
 │   2.0 ms   │ 180 Degrees (Full Right)
 └────────────┴──────────────────────────────────────────── 18.0 ms LOW ──
```

> [!IMPORTANT]
> **Pulse Width vs. Duty Cycle**: A servo does not care if the frame period drifts slightly between $18\text{ ms}$ and $22\text{ ms}$. What matters is the **exact duration of the high pulse in microseconds ($1000\ \mu\text{s} \text{ to } 2000\ \mu\text{s}$)**. A $1.50\text{ ms}$ pulse commands exactly $90.0^\circ$ regardless of whether the frame rate is $45\text{ Hz}$ or $55\text{ Hz}$.

---

### 6.3.2 Configuring an STM32 Timer for Direct Microsecond Servo Control
To eliminate floating-point math, configure an STM32 timer such that **each counter tick equals exactly $1.0\ \mu\text{s}$**.

For STM32F103 ($72\text{ MHz}$ Core Clock):
1. **Prescaler (PSC)**: Divide $72\text{ MHz}$ to $1\text{ MHz}$ ($1\text{ tick} = 1.0\ \mu\text{s}$):
   $$\text{PSC} = \frac{72{,}000{,}000}{1{,}000{,}000} - 1 = \mathbf{71}$$
2. **Auto-Reload Register (ARR)**: Create a $20\text{ ms}$ ($20{,}000\ \mu\text{s}$) frame period:
   $$\text{ARR} = 20{,}000 - 1 = \mathbf{19{,}999}$$
3. **Capture/Compare Register (CCR)**:
   - Setting $\text{CCR} = 1000 \implies 1000\ \mu\text{s} \implies 0^\circ$
   - Setting $\text{CCR} = 1500 \implies 1500\ \mu\text{s} \implies 90^\circ$
   - Setting $\text{CCR} = 2000 \implies 2000\ \mu\text{s} \implies 180^\circ$

**Listing 6.2 — Precision servo angle positioning API**

```c
#include "main.h"

extern TIM_HandleTypeDef htim2;

void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

/**
 * @brief Sets servo shaft angle in degrees.
 * @param angle_deg Desired position from 0.0 to 180.0 degrees.
 */
void Servo_SetAngle(float angle_deg)
{
    if (angle_deg < 0.0f)   angle_deg = 0.0f;
    if (angle_deg > 180.0f) angle_deg = 180.0f;

    /* Map 0.0 - 180.0 degrees to 1000 - 2000 microseconds */
    uint32_t pulse_us = (uint32_t)(1000.0f + (angle_deg * (1000.0f / 180.0f)));

    /* Write directly to Timer CCR register */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse_us);
}
```

---

## 6.4 Stepper Motor Interfacing and Motion Profiles

A bipolar stepper motor (such as a NEMA 17) consists of multiple electromagnetic stator coils arranged around a permanent magnet toothed rotor. By energizing coils in sequence, the shaft rotates in discrete, repeatable angular increments (typically $200\text{ full steps per revolution} \implies 1.8^\circ\text{ per step}$).

### 6.4.1 Stepper Drivers (A4988 / DRV8825) and Microstepping
Directly switching H-bridges to control stepper phase coils in software is cumbersome. Modern robotics designs utilize dedicated Step/Direction translator ICs: the **Allegro A4988** or **Texas Instruments DRV8825**.

```text
 STM32 Microcontroller                     A4988 / DRV8825 Driver
┌────────────────────┐                    ┌────────────────────┐
│      Timer (PA8) ──┼─── STEP Pulse ────►│ STEP               │───► Motor Coil A
│       GPIO (PB12) ─┼─── DIR Level  ────►│ DIR                │───► Motor Coil B
│       GPIO (PB13) ─┼─── ENABLE     ────►│ ENABLE             │
│                    │                    │ MS1, MS2, MS3      │ (Configures Microstep)
└────────────────────┘                    └────────────────────┘
```

#### Why Microstepping is Mandatory
In **Full-Step Mode**, the motor snaps violently between stator poles, exciting mechanical resonances that cause high audible noise, severe vibration, and lost steps.
Drivers like the A4988 and DRV8825 implement **Microstepping** by modulating current into both coils using internal DACs, positioning the magnetic field at fractional positions:

**Table 6.4 — Microstepping truth table on A4988 / DRV8825**

| MS1 Pin | MS2 Pin | MS3 Pin | Microstep Resolution | Steps per Revolution (1.8° Motor) | Motion Characteristics |
|---|---|---|---|---|---|
| LOW | LOW | LOW | **Full Step** ($1/1$) | 200 | Maximum torque; violent vibration and acoustic noise |
| HIGH | LOW | LOW | **Half Step** ($1/2$) | 400 | Reduced resonance |
| LOW | HIGH | LOW | **Quarter Step** ($1/4$)| 800 | Smooth, quiet |
| HIGH | HIGH | LOW | **Eighth Step** ($1/8$) | 1600 | Very quiet motion |
| HIGH | HIGH | HIGH | **Sixteenth Step** ($1/16$)| **3200** | **Silky-smooth, near-silent operation** |

---

### 6.4.2 The Physics of Stepper Stalling and Trapezoidal Acceleration
A stepper motor cannot start instantly at full speed. Because the motor rotor and mechanical payload possess physical inertia ($J$), attempting to step from $0\text{ steps/s}$ to $2000\text{ steps/s}$ instantaneously will cause the rotor to stall, vibrating violently in place without turning.

To reach high speeds without missing steps, firmware must implement a **Trapezoidal Velocity Profile**:

```text
 Step Rate (Hz)
      ▲
      │             Constant Velocity (Cruising)
V_max ├───         ┌─────────────────────────┐
      │           /                           \
      │          /                             \
      │         / (Acceleration)                \ (Deceleration)
V_min ├────────┘                                 └───────
      └───────────────────────────────────────────────────► Time
```

1. **Acceleration Phase**: Start at a low starting step frequency ($V_{\min} \approx 100\text{ Hz}$) and linearly increase pulse frequency at constant acceleration $a$.
2. **Cruising Phase**: Run at maximum target frequency ($V_{\max}$).
3. **Deceleration Phase**: Linearly ramp pulse frequency back down to $V_{\min}$ before coming to a complete stop.

---

## 6.5 Power Electronics: Inductive Kickback and Protection

Every electric motor, relay, and solenoid consists of tightly wound coils of wire—forming an **Inductor ($L$)**.
The fundamental physics governing an inductor is:

$$V_L = -L \frac{di}{dt}$$

An inductor violently resists any change in electrical current:
- While a transistor is ON, current flows steadily through the motor coil, establishing a strong magnetic field.
- The instant the transistor turns OFF, current tries to drop to zero ($\frac{di}{dt} \rightarrow -\infty$).
- The collapsing magnetic field induces an enormous reverse voltage spike (**Inductive Kickback / Back-EMF**) that can reach **hundreds of volts** in a fraction of a microsecond!
- This high-voltage spike will instantly arc across and destroy the switching MOSFET or BJT.

---

### 6.5.1 The Flyback (Freewheeling) Diode
To safely dissipate this inductive energy, a **Flyback Diode** is connected in parallel across the inductive load (pointing against the normal direction of power supply current):

```text
 +VMOTOR ────────────┬────────────────────────┐
                     │                        │
                   ┌─┴─┐                   ┌──┴──┐
                   │   │ Motor Coil        │     │  Flyback Diode
                   │ M │ (Inductance L)    ▲  ▲  │  (Cathode to +VMOTOR,
                   │   │                   │──│──│   Anode to Drain)
                   └─┬─┘                   └──┬──┘
                     │                        │
                     ├────────────────────────┘
                     │
                    [D] N-Channel Logic-Level MOSFET
              PA8 ──[G] (IRLZ44N / AO3400)
                    [S]
                     │
                    GND
```

When the transistor turns OFF:
1. The voltage at the transistor's Drain spikes above $+V_{\text{MOTOR}}$.
2. The flyback diode becomes **forward-biased**, turning ON.
3. The stored inductive current recirculates harmlessly in a closed loop through the motor coil and diode until its energy dissipates as heat across the winding resistance.
- *Selection Rule*: Standard 1N4001 diodes are far too slow. **Always use high-speed Schottky diodes (such as the 1N5819 or SS34)**, which turn on in sub-nanosecond timescales and feature low forward voltage drop ($V_F \approx 0.3\text{ V}$).

---

### 6.5.2 Logic-Level MOSFET Switching Rules

When using discrete transistors to switch motors, solenoids, or heaters from an STM32:
1. **Verify $V_{GS(\text{th})}$ (Logic-Level Gate Threshold)**:
   Standard power MOSFETs (such as the classic IRF540) require a gate voltage of $10\text{ V}$ to fully saturate! Driven with only $3.3\text{ V}$ from an STM32 pin, an IRF540 operates in its linear resistive region, dropping voltage and burning up.
   **Always specify true Logic-Level MOSFETs** (e.g., IRLZ44N, AO3400, FQP30N06L), which achieve minimum on-resistance ($R_{DS(\text{on})} < 0.03\ \Omega$) with $V_{GS} = 3.3\text{ V}$ or $2.5\text{ V}$.
2. **Gate Resistor ($R_{\text{gate}}$)**: Place a $47\ \Omega \text{ to } 100\ \Omega$ resistor in series between the STM32 pin and the MOSFET gate. A MOSFET gate acts as a non-linear capacitor ($\sim 1\text{ nF}$); without a series resistor, charging the gate causes a momentary $> 50\text{ mA}$ current spike, damaging the STM32 pin.
3. **Pull-Down Resistor ($R_{\text{pull}}$)**: Place a $10\text{ k}\Omega$ resistor between the MOSFET gate and GND. During microcontroller power-on, reset, or bootloader execution, GPIO pins float in high-impedance mode. Without a pull-down resistor, ambient electrostatic charges will turn the MOSFET partially ON, causing actuators to spin wildly during boot!
