# 4. Topic 3 — Low-Power Modes and Power Management

In autonomous mobile robotics, environmental sensor beacons, agricultural IoT nodes, and wearable telemetry tags, power is a finite resource. A microcontroller operating continuously at full clock frequency ($72\text{ MHz}$ on STM32F103 or $168\text{ MHz}$ on STM32F405) consumes between $30\text{ mA}$ and $70\text{ mA}$. If powered by a standard $1000\text{ mAh}$ lithium-polymer battery, the device will fully deplete its charge in **under 24 to 36 hours**.

However, real-world monitoring systems rarely need to compute at maximum capacity 100% of the time. An environmental node may only need to sample temperature, pressure, and GPS coordinates once every 10 seconds—a task requiring only $5\text{ milliseconds}$ of active processing.

By configuring the STM32's built-in **Low-Power Modes (Sleep, Stop, and Standby)** and implementing **Periodic Duty Cycling**, the average current consumption can be reduced by more than **$99\%$**, extending battery lifespan from days to **years**.

This chapter analyzes the physics of dynamic and static power dissipation, provides an exhaustive comparison of the three primary low-power modes, implements verified HAL sleep and wake-up sequences, solves the critical post-wake clock restoration problem, and presents numerical power budgeting models.

![Figure 3.1 — STM32 low-power operating modes transition state machine and duty-cycling current profile](diagrams_adv/fig3_low_power_modes.png)
*Figure 3.1 — (A) Low-power state machine comparing Run, Sleep, Stop, and Standby modes, showing clock states, SRAM retention, and wake-up triggers; (B) Periodic duty-cycling profile contrasting active bursts ($35\text{ mA}$) with deep Stop-mode sleep ($20\ \mu\text{A}$).*

---

## 4.1 Physics of Microcontroller Power Dissipation

Total electrical power consumed by a CMOS microcontroller is governed by the sum of dynamic switching power and static leakage power:

$$P_{\text{total}} = P_{\text{dynamic}} + P_{\text{static}}$$

### 4.1.1 Dynamic Switching Power ($P_{\text{dynamic}}$)
Dynamic power is consumed by charging and discharging the parasitic gate and interconnect capacitances ($C_{\text{load}}$) of millions of internal transistors on every clock transition, plus internal short-circuit crowbar current during logic switching:

$$P_{\text{dynamic}} = C_{\text{load}} \times V_{DD}^2 \times f_{\text{SYSCLK}}$$

From this governing physical equation, three vital engineering principles emerge:
1. **Linear Frequency Scaling**: Halving the CPU clock frequency (e.g., from $72\text{ MHz}$ down to $36\text{ MHz}$) cuts dynamic power consumption exactly in half.
2. **Quadratic Voltage Dependence**: Dynamic power drops with the *square* of the supply voltage ($V_{DD}^2$). Running a microcontroller at $2.0\text{ V}$ instead of $3.3\text{ V}$ reduces dynamic power consumption by:
   $$\frac{2.0^2}{3.3^2} = \frac{4.0}{10.89} \approx 36.7\%\quad (\text{A 63.3\% power reduction!})$$
3. **Peripheral Clock Gating**: An unclocked peripheral consumes zero dynamic power. If the clock bit in `RCC->APB1ENR` or `RCC->APB2ENR` is disabled, the peripheral's internal flip-flops stop toggling completely.

---

### 4.1.2 Static Leakage Power and Floating Pin Traps ($P_{\text{static}}$)
Static power represents parasitic subthreshold leakage current ($I_{\text{leak}}$) flowing across reverse-biased p-n junctions even when the core is completely halted:

$$P_{\text{static}} = V_{DD} \times I_{\text{leak}}$$

Static leakage increases exponentially with silicon junction temperature. On modern 90 nm and 40 nm silicon processes (such as STM32F4/F7/H7), static leakage is significantly higher than on older 180 nm nodes (STM32F1).

#### The Floating GPIO Power Trap
A catastrophic novice mistake in low-power firmware design is leaving unused microcontroller pins configured as **floating digital inputs (`GPIO_MODE_INPUT` without pull-up or pull-down)**.
- A floating PCB trace or open pin picks up electrostatic charges, causing its input voltage to drift to the midpoint threshold ($\sim 1.65\text{ V}$).
- At this intermediate voltage, **both the internal PMOS and NMOS transistors of the digital input Schmitt trigger turn partially ON simultaneously**.
- This creates a continuous short-circuit path between $V_{DD}$ and GND (crowbar shoot-through), bleeding **hundreds of microamperes per pin**!

> [!CAUTION]
> **The Low-Power GPIO Golden Rule**: Before placing an STM32 into Stop or Standby mode, **every unused GPIO pin must be configured in Analog Mode (`GPIO_MODE_ANALOG`)**. Analog mode completely disconnects the digital input Schmitt trigger buffer and disables the output driver, reducing pin leakage current to below $1\text{ nA}$.

---

## 4.2 STM32 Low-Power Modes: Sleep, Stop, and Standby

The STM32 Power Controller (`PWR`) provides three distinct power-saving operating modes designed to balance current reduction against wake-up latency.

**Table 4.1 — Comprehensive comparison of STM32 low-power modes**

| Dimension | Run Mode | Sleep Mode | Stop Mode | Standby Mode |
|---|---|---|---|---|
| **CPU Core** | Active (Running instructions) | **Halted** (`WFI` / `WFE`) | **Halted** | **Completely Powered OFF** |
| **Main Clocks (HSE, HSI, PLL)** | ON | ON | **OFF** | **OFF** |
| **Low-Speed Clocks (LSE, LSI)** | ON | ON | ON (Optional for RTC) | ON (Optional for RTC / IWDG) |
| **Core Regulator (1.8V / 1.2V)** | ON | ON | **Low-Power Mode (LPVR)** | **OFF** |
| **SRAM1 / SRAM2 Contents** | Retained | Retained | **FULLY RETAINED** | **LOST** (Except Backup SRAM on F4) |
| **Peripheral Registers** | Retained | Retained | **FULLY RETAINED** | **LOST** (Reset to default) |
| **Active Peripherals** | All enabled | All enabled (DMA, Timers, UART active) | **None** (Except RTC and IWDG) | **None** (Except RTC and IWDG) |
| **Typical Current (STM32F103)** | $\sim 30 - 50\text{ mA}$ | $\sim 10 - 15\text{ mA}$ | **$\sim 15 - 25\ \mu\text{A}$** | **$\sim 2 - 3\ \mu\text{A}$** |
| **Typical Current (STM32F405)** | $\sim 40 - 70\text{ mA}$ | $\sim 15 - 25\text{ mA}$ | **$\sim 100 - 250\ \mu\text{A}$** | **$\sim 2.5 - 5\ \mu\text{A}$** |
| **Wake-Up Triggers** | Not applicable | **Any enabled Interrupt or Event** | **Any EXTI Line** (Pins, RTC Alarm) | **WKUP Pin** (PA0), **RTC Alarm**, **NRST** |
| **Wake-Up Latency** | Zero | **$0.25\ \mu\text{s}$** (Clock running) | **$\sim 5 - 10\ \mu\text{s}$** (Oscillator start) | **$\sim 50 - 100\ \mu\text{s}$** (Full reset reboot) |
| **Execution After Wake-Up** | Continuous | Resumes next instruction after `WFI` | **Resumes next instruction** (On HSI!) | **Executes `Reset_Handler` (Reboots)** |

---

### 4.2.1 Sleep Mode: Peripheral-Driven Background Processing
In Sleep mode, only the ARM Cortex-M processor core stops executing instructions; all internal peripheral buses, DMA controllers, timers, and Flash interfaces remain fully clocked and active.
- *Best Used For*: Waiting for ADC DMA completions or high-speed UART packet reception without wasting CPU cycles in busy-wait polling loops.
- *Entry*: Execute the Wait-For-Interrupt (`WFI`) assembly instruction via `HAL_PWR_EnterSLEEPMode()`.
- *Wake-Up*: Any enabled peripheral interrupt wakes the CPU immediately with sub-microsecond latency.

---

### 4.2.2 Stop Mode: Deep Sleep with Full SRAM Retention
Stop mode achieves the optimal compromise between extreme power reduction ($\sim 20\ \mu\text{A}$) and fast state restoration.
- All high-frequency oscillators (HSE, HSI, PLL) are shut down.
- The internal core voltage regulator is placed into low-power mode.
- **SRAM contents and all peripheral register configurations are completely preserved**.
- *Best Used For*: Battery-powered sensor nodes that wake up periodically (e.g. every 1 second or 10 seconds via RTC) to perform measurements, or robots waiting for an external button press.
- *Wake-Up*: Triggered by any external interrupt line (`EXTI0` through `EXTI15`), the RTC Alarm, or USB wake-up.

---

### 4.2.3 Standby Mode: Ultra-Low-Power Deep Sleep
Standby mode powers down the entire internal 1.8V/1.2V core logic domain.
- The internal voltage regulator is completely turned OFF.
- **All SRAM contents and peripheral registers are permanently lost**. Only the Backup Domain (RTC registers and Backup SRAM) remains powered via $V_{\text{BAT}}$.
- *Best Used For*: Devices that remain turned off for weeks or months (e.g., an emergency tracking beacon) and only wake up when a user presses a physical "Power/Wake" button or an RTC calendar event expires.
- *Wake-Up*: Dedicated `WKUP` pin (PA0 rising edge), RTC Alarm, external `NRST` pin, or IWDG reset.
- *Wake-Up Behavior*: The microcontroller reboots from scratch, executing `Reset_Handler` exactly like a cold power-on.

---

## 4.3 Practical HAL Implementation and the Post-Wake Clock Trap

### 4.3.1 Entering Sleep Mode

```c
/* Put CPU to sleep; wakes up instantly upon any enabled interrupt (e.g. SysTick, UART) */
HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
```

---

### 4.3.2 Entering Stop Mode and the Critical Clock Restoration Sequence

When an STM32 wakes up from Stop mode, **hardware automatically defaults to running on the internal low-speed RC oscillator (HSI: 8 MHz on F1, 16 MHz on F4)**!
- The high-speed external crystal (HSE) and PLL **remain completely turned OFF**.
- If firmware immediately resumes executing without reconfiguring clocks, the microcontroller will run at $8\text{ MHz}$ instead of $72\text{ MHz}$ or $168\text{ MHz}$.
- **Consequence**: All UART baud rates, I²C clocks, and timer PWM frequencies will be completely corrupted!

> [!WARNING]
> **Mandatory Post-Stop Clock Restoration**: Immediately following the `HAL_PWR_EnterSTOPMode()` call, firmware **MUST explicitly re-enable the PLL and restore the high-speed system clock** by invoking `SystemClock_Config()`.

**Listing 4.1 — Verified Stop-mode entry and clock restoration sequence**

```c
#include "main.h"

void App_EnterDeepSleep(void)
{
    /* 1. Suspend SysTick to prevent 1 ms tick interrupts from immediately waking the core */
    HAL_SuspendTick();

    /* 2. Configure all non-essential GPIO pins to Analog Mode to eliminate pin leakage */
    /* (Ensure wake-up EXTI pins remain configured as inputs!) */

    /* 3. Enter Stop Mode with Low-Power Regulator */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* =========================================================================
       SYSTEM IS IN STOP MODE (CONSUMING ~20 µA).
       CPU EXECUTION HALTS HERE UNTIL AN EXTI PIN OR RTC ALARM FIRES.
       ========================================================================= */

    /* 4. CRITICAL: Hardware woke up on internal 8 MHz HSI! Reconfigure PLL & HSE */
    SystemClock_Config();

    /* 5. Resume SysTick timer for HAL delays */
    HAL_ResumeTick();
}
```

---

### 4.3.3 Configuring an External Pin (EXTI) Wake-Up Trigger

To wake the STM32 from Stop mode using an external button or sensor interrupt pin:
1. In CubeMX, set the target pin (e.g., `PA0` or `PB12`) to **`GPIO_EXTI`**.
2. Under GPIO settings:
   - **Mode**: `External Interrupt Mode with Falling edge trigger detection`.
   - **Pull-up/Pull-down**: `Pull-up` (if active-low button).
3. Under NVIC settings: **Enable EXTI line interrupt**.
4. Implement the ISR callback:

```c
/* Weak callback invoked when wake-up pin generates a falling edge */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        /* User pressed wake-up button! */
    }
}
```

---

### 4.3.4 Entering Standby Mode

```c
void App_EnterStandby(void)
{
    /* 1. Clear Standby and Wake-Up flags */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* 2. Enable WKUP pin (PA0 on STM32F1/F4) */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

    /* 3. Enter Standby Mode (SRAM will be wiped; reboots upon wake-up) */
    HAL_PWR_EnterSTANDBYMode();
}
```

---

## 4.4 Mathematical Power Budgeting and Battery Lifetime

When designing battery-powered robotic nodes, engineers must calculate the **average current consumption ($I_{\text{avg}}$)** over an operational cycle.

### 4.4.1 The Duty-Cycling Governing Equation

Consider a system operating in periodic bursts:
- **Active Period ($T_{\text{active}}$)**: The system wakes up, samples sensors, executes control math, and transmits radio telemetry, consuming high active current ($I_{\text{active}}$).
- **Sleep Period ($T_{\text{sleep}}$)**: The system enters deep Stop mode, consuming quiescent leakage current ($I_{\text{sleep}}$).

The average current is:

$$I_{\text{avg}} = \frac{(T_{\text{active}} \times I_{\text{active}}) + (T_{\text{sleep}} \times I_{\text{sleep}})}{T_{\text{active}} + T_{\text{sleep}}}$$

The **Duty Cycle ($D$)** is defined as:

$$D = \frac{T_{\text{active}}}{T_{\text{active}} + T_{\text{sleep}}}$$

$$I_{\text{avg}} = D \times I_{\text{active}} + (1 - D) \times I_{\text{sleep}}$$

---

### 4.4.2 Battery Operating Lifetime Calculation

Given a battery nominal capacity $C_{\text{batt}}$ (in milliampere-hours, $\text{mAh}$) and a real-world derating factor $\eta$ (typically $0.85$ to account for battery self-discharge and DC-DC converter inefficiency):

$$\text{Operating Life (Hours)} = \frac{C_{\text{batt}} \times \eta}{I_{\text{avg}}}$$

$$\text{Operating Life (Days)} = \frac{\text{Operating Life (Hours)}}{24}$$

#### Worked Numerical Comparison: Environmental Sensor Node
A remote agricultural robot node monitors soil moisture and temperature powered by a standard $1000\text{ mAh}$ LiPo battery ($\eta = 0.85 \implies \text{Effective Capacity} = 850\text{ mAh}$).
- When awake: $I_{\text{active}} = 35\text{ mA}$. Active task takes $T_{\text{active}} = 10\text{ ms}$ ($0.010\text{ s}$).
- Measurement period: Once every $10\text{ seconds}$ ($T_{\text{sleep}} = 9.990\text{ s}$).

#### Case 1: Amateur Design (Continuous Run Mode with `HAL_Delay`)
The CPU spins continuously in Run mode at $72\text{ MHz}$ during the 10-second wait:
$$I_{\text{avg}} = 35\text{ mA}$$
$$\text{Battery Life} = \frac{850\text{ mAh}}{35\text{ mA}} \approx 24.3\text{ hours (Just ONE DAY!)}$$

#### Case 2: Professional Design (Stop Mode with RTC Wakeup)
The CPU enters Stop mode ($I_{\text{sleep}} = 20\ \mu\text{A} = 0.020\text{ mA}$) during the 9.990 seconds:
$$I_{\text{avg}} = \frac{(0.010\text{ s} \times 35\text{ mA}) + (9.990\text{ s} \times 0.020\text{ mA})}{10.0\text{ s}} = \frac{0.35 + 0.1998}{10.0} \approx 0.055\text{ mA} = 55\ \mu\text{A}$$
$$\text{Battery Life} = \frac{850\text{ mAh}}{0.055\text{ mA}} \approx 15{,}454\text{ hours} \approx \mathbf{643.9\text{ days (1.76 YEARS!)}}$$

> [!TIP]
> **Power Optimization Rule**: Implementing Stop mode transformed battery life from **24 hours to nearly 2 years** on the exact same battery—a **$636\times$ improvement in operating endurance**!
