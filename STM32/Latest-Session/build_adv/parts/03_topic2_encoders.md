# 3. Topic 2 — Hardware Quadrature Encoder Interface Using STM32 Timers

In robotics and precision motion control, a microcontroller must know the exact angular position, rotational velocity, and travel distance of motor shafts and robot wheels. Attempting to measure motor speed by polling GPIO pins or counting external interrupts (`EXTI`) in software fails catastrophically: at high rotational speeds, encoder signals generate tens of thousands of pulses per second, swamping the CPU with interrupt overhead, inducing severe timing jitter, and dropping counts.

To solve this problem, STM32 general-purpose and advanced timers (TIM1, TIM2, TIM3, TIM4, TIM5, TIM8) feature a dedicated **Hardware Quadrature Encoder Interface**. The timer hardware decodes 2-channel quadrature signals directly in silicon, incrementing or decrementing its hardware counter with **zero CPU utilization**.

This chapter provides a complete engineering treatment of quadrature encoding, derives the mathematical governing equations for odometry, presents the two's complement overflow-handling algorithm, and builds a production-grade motor velocity and position measurement driver.

![Figure 2.1 — Quadrature waveforms and STM32 timer hardware encoder interface](diagrams_adv/fig2_quadrature_encoder.png)
*Figure 2.1 — (A) Quadrature waveforms showing 90° phase shift in forward (clockwise) and reverse (counter-clockwise) rotation, with x4 edge transition counts; (B) STM32 timer hardware encoder block diagram showing digital glitch filters (IC1F/IC2F), edge decoders, and bidirectional counter.*

---

## 3.1 Incremental Rotary Encoder Fundamentals

An incremental rotary encoder converts mechanical shaft rotation into a stream of electrical pulses. It consists of a rotating code disc (optical glass or magnetic multipole ring) and a stationary sensing head (optical photodiodes or Hall-effect sensors).

### 3.1.1 Optical vs. Magnetic Encoders
- **Optical Encoders**: Use a transparent disc with etched chrome lines interrupting an infrared LED beam. Provide exceptional angular resolution ($500\text{ to } 5000+\text{ pulses per revolution}$), but are sensitive to dirt, dust, and mechanical vibration.
- **Magnetic Encoders**: Use a diametrically magnetized disc positioned over Hall-effect or AMR/GMR sensor ICs (e.g., AS5048A, AS5600). Highly rugged, impervious to oil and dust, but subject to magnetic interference from motor stator windings.

---

### 3.1.2 Quadrature Encoding: The $90^\circ$ Phase Shift

An incremental encoder outputs two digital square-wave signals: **Channel A** and **Channel B**.
The physical sensor heads are positioned such that the electrical waveforms are **spatially shifted by $90^\circ$ (one quarter of a cycle, or $\frac{\pi}{2}$ radians)**. This phase relationship is termed **Quadrature**.

```text
Forward Rotation (CW): Channel A LEADS Channel B by 90°
Channel A:  ───┐       ┌───────┐       ┌───────┐
               └───────┘       └───────┘       └───────
Channel B:  ───────┐       ┌───────┐       ┌───────┐
                   └───────┘       └───────┘       └───
State:      [00] [10]   [11]   [01]   [00] ...

Reverse Rotation (CCW): Channel B LEADS Channel A by 90°
Channel A:  ───────┐       ┌───────┐       ┌───────┐
                   └───────┘       └───────┘       └───
Channel B:  ───┐       ┌───────┐       ┌───────┐
               └───────┘       └───────┘       └───────
State:      [00] [01]   [11]   [10]   [00] ...
```

#### Direction Detection Principle
By sensing the level of one channel during the transition edge of the other, direction is determined unambiguously:
- In **Forward Rotation**: When Channel A transitions from LOW to HIGH, Channel B is **LOW**.
- In **Reverse Rotation**: When Channel A transitions from LOW to HIGH, Channel B is **HIGH**.

---

### 3.1.3 Resolution: Pulses Per Revolution (PPR) vs. Counts Per Revolution (CPR)

One of the most persistent terminology confusions in robotics is the distinction between **PPR** and **CPR**:
- **Pulses Per Revolution (PPR)**: The number of complete electrical square-wave cycles generated on a single channel per one mechanical revolution of the shaft.
- **Counts Per Revolution (CPR)**: The total number of decoded digital position increments registered by the microcontroller per revolution.

The relationship depends entirely on the timer decoding mode:

**Table 3.1 — Encoder decoding modes and effective resolution multiplier**

| Decoding Mode | Edges Counted by Hardware | Resolution Multiplier | Effective CPR | Best Use Case |
|---|---|---|---|---|
| **x1 Mode** | Rising edges of Channel A only | $1\times$ | $\text{CPR} = 1 \times \text{PPR}$ | Coarse velocity measurement |
| **x2 Mode** | Rising and falling edges of Channel A | $2\times$ | $\text{CPR} = 2 \times \text{PPR}$ | Medium resolution |
| **x4 Mode (TI1 and TI2)** | **Both rising and falling edges of BOTH Ch A and Ch B** | **$4\times$** | **$\text{CPR} = 4 \times \text{PPR}$** | **Universal Robotics Standard** (Maximum resolution & lowest jitter) |

#### Worked Example: Optical Encoder Resolution
An encoder has a physical rating of $500\text{ PPR}$. When connected to an STM32 timer in **x4 mode (`Encoder Mode TI1 and TI2`)**:

$$\text{CPR} = 4 \times 500 = 2000\text{ counts per revolution}$$
$$\text{Angular Resolution} = \frac{360^\circ}{2000} = 0.18^\circ\text{ per count}$$

If the motor drives a $65\text{ mm}$ robot wheel directly, the linear position resolution is:

$$\text{Linear Resolution} = \frac{\pi \times D}{\text{CPR}} = \frac{\pi \times 0.065\text{ m}}{2000} \approx 0.1021\text{ mm per tick}$$

---

## 3.2 STM32 Hardware Timer Encoder Mode

Inside the STM32 timer peripheral (TIM1, TIM2, TIM3, TIM4, TIM5, TIM8), input channels **`CH1` (TI1)** and **`CH2` (TI2)** feed directly into an internal hardware **Quadrature State Decoder**:

```text
Pin CH1 (TI1) ──► Input Filter (IC1F) ──► Polarity (CC1P) ──┐
                                                             ├──► Encoder Decoder ──► Up/Down ──► CNT Register
Pin CH2 (TI2) ──► Input Filter (IC2F) ──► Polarity (CC2P) ──┘    (SMS[2:0]=011)        Clock
```

### 3.2.1 Digital Noise Glitch Filtering (`IC1F` / `IC2F`)
DC motors emit significant electrical brush noise and inductive spikes that can couple into encoder wiring. A noise spike appearing on an encoder line can cause false edges, resulting in positioning drift.

The STM32 timer inputs include configurable digital glitch filters (`IC1F` and `IC2F` in the `TIMx_CCMR1` register). The filter samples the physical pin using an internal clock derived from the timer clock ($f_{\text{DTS}}$). The output transitions only after the pin holds its new state steadily for $N$ consecutive samples (e.g., $N = 6$ or $8$).
- In CubeMX: Set **Input Filter = 6 to 10** for noisy industrial environments.

---

### 3.2.2 Step-by-Step STM32CubeMX Configuration

To configure a timer (e.g., `TIM2` or `TIM3`) as a hardware quadrature encoder interface:

1. Open the `.ioc` file in STM32CubeMX or STM32CubeIDE.
2. Select **Timers → TIM2** (or `TIM3` / `TIM4`).
3. Set **Combined Channels** to **`Encoder Mode`**.
4. In the **Parameter Settings** tab:
   - **Encoder Mode**: Select **`Encoder Mode TI1 and TI2`** (Enables $4\times$ decoding).
   - **Counter Settings**:
     - **Prescaler (PSC)**: Set strictly to **`0`** (No clock division; count every edge).
     - **Counter Period (ARR)**: Set to **`65535`** (`0xFFFF` for 16-bit timers) or **`4294967295`** (`0xFFFFFFFF` for 32-bit timers TIM2/TIM5 on STM32F4).
     - **Counter Mode**: `Up` (The hardware decoder will override direction automatically).
   - **Input Channel 1 (TI1)**:
     - **Polarity**: `Rising Edge`
     - **Input Filter**: `8` (Rejects high-frequency motor brush noise).
   - **Input Channel 2 (TI2)**:
     - **Polarity**: `Rising Edge`
     - **Input Filter**: `8`
5. Note the automatically assigned pins (e.g., `PA0 = TIM2_CH1`, `PA1 = TIM2_CH2` on STM32F1/F4).
6. Click **Generate Code**.

---

## 3.3 Production HAL Implementation

**Listing 3.1 — Initializing and reading the hardware encoder**

```c
#include "main.h"

extern TIM_HandleTypeDef htim2;

void Encoder_Init(void)
{
    /* Start the hardware encoder interface on both channels */
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    
    /* Initialize counter to 0 */
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

uint16_t Encoder_GetRawCount(void)
{
    /* Read the 16-bit hardware counter register (CNT) */
    return (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
}

bool Encoder_IsCountingDown(void)
{
    /* Check hardware direction bit (TIM_CR1_DIR: 0 = Up/Forward, 1 = Down/Reverse) */
    return __HAL_TIM_IS_TIM_COUNTING_DOWN(&htim2);
}
```

---

## 3.4 Mathematical Derivations: Displacement, RPM, and Velocity

To use encoder readings in closed-loop control or robot navigation, raw count ticks must be converted into physical SI engineering units.

### 3.4.1 Mathematical Governing Equations

#### 1. Angular Displacement ($\Delta \theta$)
The change in shaft angle over sample interval $\Delta t$:

$$\Delta \theta = \frac{\Delta \text{Count}}{\text{CPR}} \times 2\pi\quad [\text{radians}]$$
$$\Delta \theta = \frac{\Delta \text{Count}}{\text{CPR}} \times 360^\circ\quad [\text{degrees}]$$

#### 2. Rotational Velocity (RPM)
Rotational speed in revolutions per minute:

$$\text{RPM} = \frac{\Delta \text{Count}}{\text{CPR}} \times \frac{60}{\Delta t}$$

Where:
- $\Delta \text{Count}$ is the signed difference in encoder counts over interval $\Delta t$.
- $\text{CPR}$ is the decoded counts per revolution ($4 \times \text{PPR}$).
- $\Delta t$ is the measurement sampling interval in seconds (e.g., $0.020\text{ s}$ for a 50 Hz control loop).

#### 3. Linear Wheel Velocity ($v$)
For a mobile robot with wheel diameter $D$ (meters):

$$v = \omega \times r = \left(\text{RPM} \times \frac{2\pi}{60}\right) \times \frac{D}{2} = \frac{\pi \times D \times \text{RPM}}{60}\quad [\text{m/s}]$$

Substituting the RPM equation:

$$v = \frac{\pi \times D \times \Delta \text{Count}}{\text{CPR} \times \Delta t}\quad [\text{m/s}]$$

#### 4. Total Traveled Distance ($s$)

$$s = \frac{\text{Total Net Count}}{\text{CPR}} \times (\pi \times D)\quad [\text{meters}]$$

---

## 3.5 The 16-Bit Counter Rollover and Overflow Handling

A major trap for embedded students is the **16-bit timer overflow**. 
On STM32F103 (and most timers on STM32F4), the counter register `CNT` is **16 bits wide**, spanning values from `0` to `65535` (`0xFFFF`):
- When spinning forward: Counter reaches `65535` and rolls over to `0`.
- When spinning in reverse: Counter reaches `0` and rolls under to `65535`.

If firmware calculates speed naively as `delta = current_cnt - prev_cnt`, an overflow produces catastrophic errors:
- Example: Motor spins forward. `prev_cnt = 65530`, `curr_cnt = 10`.
- Naive unsigned subtraction: $10 - 65530 = \text{Underflow}$!
- In integer math: $10 - 65530 = -65520$. The robot suddenly thinks it is spinning backwards at thousands of RPM, causing the PID controller to slam the motors to full reverse!

---

### 3.5.1 The Signed 16-Bit Integer Cast Trick

In C on 32-bit microcontrollers, this problem is solved elegantly in **a single line of code with zero branching** by leveraging two's complement modular arithmetic:

```c
/* CORRECT: Two's complement signed cast handles overflow automatically */
int16_t delta_counts = (int16_t)(current_cnt - prev_cnt);
```

#### Mathematical Proof
Let the counter register be 16 bits wide ($M = 2^{16} = 65536$).
1. **Forward Rollover Example**:
   - `prev_cnt` = $65530$ (`0xFFFA`)
   - `curr_cnt` = $10$ (`0x000A`)
   - True physical movement = $+16\text{ counts}$ forward.
   - Unsigned 16-bit subtraction in hardware:
     $$\text{Result} = (10 - 65530) \pmod{65536} = -65520 \equiv 16\ (\text{hex: 0x0010})$$
   - When bit-pattern `0x0010` is cast to signed `int16_t`, the result is **`+16`**!
2. **Reverse Rollover Example**:
   - `prev_cnt` = $10$ (`0x000A`)
   - `curr_cnt` = $65530$ (`0xFFFA`)
   - True physical movement = $-16\text{ counts}$ reverse.
   - Unsigned 16-bit subtraction:
     $$\text{Result} = (65530 - 10) \pmod{65536} = 65520\ (\text{hex: 0xFFF0})$$
   - When bit-pattern `0xFFF0` is cast to signed `int16_t`, two's complement sign extension interprets it as **`-16`**!

> [!IMPORTANT]
> **The Safe Sampling Frequency Constraint (The Nyquist Limit of Encoders)**:
> The signed 16-bit casting trick is valid **if and only if the motor cannot rotate more than half the counter span ($\frac{65536}{2} = 32{,}768\text{ counts}$) in a single sample period $\Delta t$**:
> 
> $$|\Delta \text{Count}| < 32{,}768$$
> 
> For a high-resolution $1000\text{ PPR}$ ($4000\text{ CPR}$) motor running at $6000\text{ RPM}$ ($100\text{ rev/s} \implies 400{,}000\text{ counts/s}$):
> 
> $$\Delta t_{\max} = \frac{32{,}768}{400{,}000\text{ counts/s}} \approx 81.9\text{ ms}$$
> 
> Therefore, sampling the encoder at **$50\text{ Hz}$ or $100\text{ Hz}$ ($\Delta t = 20\text{ ms}$ or $10\text{ ms}$)** is mathematically guaranteed to prevent rollover aliasing under all operating conditions.

---

### 3.5.2 Complete Production Velocity and Odometry Driver

**Listing 3.2 — Reusable encoder velocity and position tracking module**

```c
#include "main.h"
#include <stdbool.h>

typedef struct {
    TIM_HandleTypeDef *htim;           /* Hardware timer instance (e.g. &htim2) */
    uint16_t           cpr;            /* Counts per revolution (4 * PPR) */
    float              wheel_diameter; /* Wheel diameter in meters */
    uint16_t           prev_counter;   /* Previous raw 16-bit CNT value */
    
    /* Calculated Outputs */
    int16_t            delta_ticks;    /* Count change in last sample interval */
    int64_t            total_ticks;    /* Accumulated multi-turn signed ticks */
    float              rpm;            /* Current rotational speed */
    float              linear_vel_mps; /* Linear wheel speed in m/s */
    float              total_dist_m;   /* Total distance traveled in meters */
} Encoder_t;

void Encoder_InitInstance(Encoder_t *enc, TIM_HandleTypeDef *htim, uint16_t cpr, float wheel_diameter_m)
{
    enc->htim           = htim;
    enc->cpr            = cpr;
    enc->wheel_diameter = wheel_diameter_m;
    enc->prev_counter   = 0;
    enc->delta_ticks    = 0;
    enc->total_ticks    = 0;
    enc->rpm            = 0.0f;
    enc->linear_vel_mps = 0.0f;
    enc->total_dist_m   = 0.0f;

    /* Start hardware peripheral */
    HAL_TIM_Encoder_Start(enc->htim, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(enc->htim, 0);
    enc->prev_counter = 0;
}

/**
 * @brief Periodic update function. MUST be called at a strictly regular interval!
 * @param enc Pointer to encoder instance.
 * @param dt_seconds Time elapsed since last call in seconds (e.g. 0.020f for 50 Hz).
 */
void Encoder_Update(Encoder_t *enc, float dt_seconds)
{
    if (dt_seconds <= 0.0f) return;

    /* 1. Read current hardware counter */
    uint16_t current_counter = (uint16_t)__HAL_TIM_GET_COUNTER(enc->htim);

    /* 2. Calculate signed difference using 16-bit modular casting */
    enc->delta_ticks = (int16_t)(current_counter - enc->prev_counter);
    enc->prev_counter = current_counter;

    /* 3. Accumulate total 64-bit multi-turn position */
    enc->total_ticks += enc->delta_ticks;

    /* 4. Calculate RPM */
    /* RPM = (delta_ticks / CPR) * (60 / dt) */
    enc->rpm = ((float)enc->delta_ticks / (float)enc->cpr) * (60.0f / dt_seconds);

    /* 5. Calculate Linear Velocity (m/s) */
    /* v = (pi * D * delta_ticks) / (CPR * dt) */
    enc->linear_vel_mps = (3.14159265f * enc->wheel_diameter * (float)enc->delta_ticks) / 
                          ((float)enc->cpr * dt_seconds);

    /* 6. Calculate Total Linear Distance (meters) */
    enc->total_dist_m = ((float)enc->total_ticks / (float)enc->cpr) * 
                        (3.14159265f * enc->wheel_diameter);
}
```

---

## 3.6 Robotics Applications: Differential-Drive Wheel Odometry

In a two-wheeled differential-drive mobile robot, two independent encoders track the left wheel and right wheel:

```text
       Robot Coordinates (x, y, θ)
              ▲ Y
              │
              │   Left Wheel [v_L]
              ├───[===]
              │     │  Track Gauge (L)
         ───► ┼─────┼────────► X
        (θ)   │     │
              ├───[===]
                  Right Wheel [v_R]
```

### 3.6.1 Forward Kinematics Equations
Let the distance between the two wheels be the **Track Gauge ($L$)**, and the measurement interval be $\Delta t$. From the left and right wheel distances ($\Delta s_L$ and $\Delta s_R$):

$$\Delta s = \frac{\Delta s_R + \Delta s_L}{2}\quad (\text{Linear robot displacement})$$
$$\Delta \theta = \frac{\Delta s_R - \Delta s_L}{L}\quad (\text{Angular heading change in radians})$$

The updated robot pose $(x_{k+1}, y_{k+1}, \theta_{k+1})$ in the world frame is computed using second-order Runge-Kutta integration:

$$\theta_{k+1} = \theta_k + \Delta \theta$$
$$x_{k+1} = x_k + \Delta s \times \cos\left(\theta_k + \frac{\Delta \theta}{2}\right)$$
$$y_{k+1} = y_k + \Delta s \times \sin\left(\theta_k + \frac{\Delta \theta}{2}\right)$$

---

### 3.6.2 Physical Limitations of Pure Odometry (Why Encoders Drift)
While wheel encoders provide excellent short-term velocity feedback for closed-loop motor PID control, **pure dead-reckoning odometry inevitably drifts over time**:
1. **Wheel Slip**: Rapid acceleration or braking breaks physical traction with the floor.
2. **Wheel Diameter Variations**: Uneven tire compression or manufacturing tolerances cause one wheel to travel slightly further per revolution.
3. **Caster Wheel Drag**: Passive swivel caster wheels exert asymmetric lateral forces during turns.
4. **Integration Error**: Heading error ($\Delta \theta$) accumulates unbounded; an orientation error of only $2^\circ$ leads to massive position divergence after traveling 10 meters.

> [!TIP]
> **Robotics Engineering Best Practice**: Use wheel encoders as the high-rate ($100\text{ Hz}$) velocity feedback loop for motor PID speed control. For global robot orientation and positioning, fuse wheel odometry with an onboard **IMU Gyroscope (Yaw axis)** using an **Extended Kalman Filter (EKF)**.
