# STM32 Task 3 — Full Solution

**Assiut Robotics Team — Embedded Systems Batch**  
**Target MCU:** STM32F103C8T6 (ARM Cortex-M3 "Blue Pill")  
**Project Path:** `STM32/Task3/STM32F103C8TX/STM32F103C8TX`  
**Reference Assignment:** [`Task3_STM.pdf`](./Task3_STM.pdf)  
**Proteus Project:** [`Task3-Proteus.pdsprj`](./STM32F103C8TX/STM32F103C8TX/Task3-Proteus.pdsprj)  

---

## 1. Objectives & Deliverables Summary

All requirements from `Task3_STM.pdf` are fully implemented inside this project:

| Deliverable | Requirement | Implementation in Project |
| :--- | :--- | :--- |
| **1. Ultrasonic Distance on LCD** | Read distance from Ultrasonic sensor (HC-SR04) and display it on 16x2 LCD. | Implemented in [`ultrasonic.h`](./STM32F103C8TX/STM32F103C8TX/Core/Inc/ultrasonic.h) & [`ultrasonic.c`](./STM32F103C8TX/STM32F103C8TX/Core/Src/ultrasonic.c). Uses 1&nbsp;µs hardware timer `TIM1` and dual-edge `EXTI4` capture. Rendered via 8-bit LCD driver [`lcd.c`](./STM32F103C8TX/STM32F103C8TX/Core/Src/lcd.c). |
| **2. Periodic Pulse Waveform** | Produce the custom periodic pulse waveform and inspect it on an oscilloscope. | Implemented in [`waveform.h`](./STM32F103C8TX/STM32F103C8TX/Core/Inc/waveform.h) & [`waveform.c`](./STM32F103C8TX/STM32F103C8TX/Core/Src/waveform.c). Driven by hardware `TIM2` state machine on pins `PA5` and `PB8`. Total period = $60\ \mu\text{s}$ ($f \approx 16.67\text{ kHz}$). |
| **3. Strict Super-Loop Constraint** | *"Perform the Prev task without writing any line inside the super Loop."* | Inside [`main.c`](./STM32F103C8TX/STM32F103C8TX/Core/Src/main.c), the `while (1)` loop is **100% empty**. The entire application is non-blocking and driven completely by hardware Timers and Interrupts. |
| **4. External Files & Doxygen** | *"Write the whole functions related to Ultrasonic in an external files .h/.c to enhance the readability, also use DOXIGEN doc."* | Complete modular design with separate `.h`/`.c` pairs and full Doxygen comments (`@file`, `@brief`, `@param`, `@retval`, `@details`) across all modules. |
| **5. Simulation & Submission** | Working Proteus simulation project configured with firmware. | Project configured in [`Task3-Proteus.pdsprj`](./STM32F103C8TX/STM32F103C8TX/Task3-Proteus.pdsprj) with STM32F103C8, HC-SR04 sensor, 16x2 LCD, and Oscilloscope, loaded with `Debug/STM32F103C8TX.hex`. |

---

## 2. Hardware Wiring & Pinout Guide

### Pin Connections

| Peripheral / Signal | STM32 Pin | Mode | Description |
| :--- | :--- | :--- | :--- |
| **LCD Enable (`EN`)** | `PA0` | Output Push-Pull | LCD strobe enable pulse |
| **LCD Reg Select (`RS`)** | `PA1` | Output Push-Pull | `0` = Command register, `1` = Data register |
| **LCD Read/Write (`RW`)** | `PA2` | Output Push-Pull | Pulled `0` for Write mode |
| **Ultrasonic Trigger (`TRIG`)** | `PA3` | Output Push-Pull | $10\ \mu\text{s}$ trigger pulse |
| **Ultrasonic Echo (`ECHO`)** | `PA4` | Input (`EXTI4`) | Dual-edge interrupt (Rising & Falling) |
| **Waveform Output (Ch A)** | `PA5` | Output Push-Pull | Oscilloscope Channel A signal output |
| **LCD 8-Bit Data (`DB0`..`DB7`)** | `PB0`..`PB7` | Output Push-Pull | LCD 8-bit parallel data bus |
| **Waveform Output (Ch B)** | `PB8` | Output Push-Pull | Synchronized duplicate pulse waveform output |

```
                       STM32F103C8T6 (Blue Pill)
                       +-----------------------+
                EN  <--| PA0               PB0 |---> LCD DB0
                RS  <--| PA1               PB1 |---> LCD DB1
                RW  <--| PA2               PB2 |---> LCD DB2
        HC-SR04 TRIG<--| PA3               PB3 |---> LCD DB3
        HC-SR04 ECHO-->| PA4 (EXTI4)       PB4 |---> LCD DB4
       Scope Out PA5<--| PA5               PB5 |---> LCD DB5
                       | PA6               PB6 |---> LCD DB6
                       | PA7               PB7 |---> LCD DB7
                       |                   PB8 |---> Scope Out PB8
                       +-----------------------+
```

---

## 3. System Architecture & Timer Allocation

To satisfy the **strict constraint of an empty `while (1)` loop**, all tasks operate concurrently via hardware interrupts and hardware timers:

```
[Hardware System Clock: 8 MHz HSI]
  |
  +---> TIM1 (1 MHz Free-Running Counter)
  |       └── 1 tick = 1 µs timebase for Ultrasonic Echo duration measurement
  |
  +---> EXTI4 (PA4 Dual-Edge Interrupt)
  |       ├── Rising Edge  : captures start timestamp (TIM1->CNT)
  |       └── Falling Edge : calculates pulse width = (TIM1->CNT - start)
  |                          computes distance = pulse_width / 58 cm
  |
  +---> TIM2 (1 MHz Variable Auto-Reload Timer)
  |       └── State Machine ISR (10µs LOW -> 20µs HIGH -> 10µs LOW -> 5µs HIGH -> 15µs LOW)
  |           Flips PA5 and PB8 directly via atomic GPIO BSRR registers
  |
  +---> TIM3 (100 ms Periodic Task Timer)
  |       ├── Emits 10 µs TRIG pulse on PA3
  |       └── Formats and prints measured distance to 16x2 LCD
  |
  +---> main() Super-Loop
          └── while (1) { /* 100% EMPTY */ }
```

### Hardware Timer Resource Allocation

| Hardware Peripheral | Configuration | Purpose |
| :--- | :--- | :--- |
| **TIM1** | $\text{PSC} = 7$, $\text{ARR} = 65535$ ($1\text{ MHz}$ tick = $1\ \mu\text{s}$) | Microsecond free-running timebase for high-precision echo measurement. |
| **TIM2** | $\text{PSC} = 7$, Dynamic $\text{ARR}$ with Update Interrupt (`UIE`) | 4-phase pulse waveform state machine ($60\ \mu\text{s}$ total period). |
| **TIM3** | $\text{PSC} = 7999$, $\text{ARR} = 99$ ($10\text{ Hz}$ update rate = $100\text{ ms}$) | Periodic non-blocking trigger for ultrasonic sensor and LCD screen updates. |
| **EXTI4** | Dual Edge (Rising & Falling on `PA4`) | Captures echo pulse width with sub-microsecond precision. |

---

## 4. Ultrasonic Sensor (HC-SR04) Theory & Implementation

### Operating Principle

1. The microcontroller emits a $10\ \mu\text{s}$ HIGH pulse on the `TRIG` pin.
2. The HC-SR04 transmits an 8-cycle ultrasonic burst at $40\text{ kHz}$.
3. The sensor raises the `ECHO` pin HIGH for a duration proportional to the sound wave flight time:
   $$\text{Total Distance} = \text{Speed of Sound} \times \frac{\text{Time}}{2}$$
   $$\text{Speed of Sound} \approx 340\text{ m/s} = 0.034\text{ cm}/\mu\text{s} = \frac{1}{29.1}\text{ cm}/\mu\text{s}$$
   $$\text{Distance (cm)} = \frac{\text{Pulse Width in }\mu\text{s}}{58}$$
   $$\text{Distance (inches)} = \frac{\text{Pulse Width in }\mu\text{s}}{148}$$

### Non-Blocking Edge Capture Routine

```c
void Ultrasonic_Echo_ISR(Ultrasonic_t *us, uint16_t GPIO_Pin)
{
    if (HAL_GPIO_ReadPin(us->echo.port, us->echo.pin) == GPIO_PIN_SET)
    {
        /* Rising Edge: Echo pulse has begun, record timestamp from TIM1 */
        us->echo_start = TIM1->CNT;
    }
    else
    {
        /* Falling Edge: Echo pulse ended, calculate elapsed microseconds */
        uint32_t echo_end = TIM1->CNT;
        uint32_t pulse_width;

        if (echo_end >= us->echo_start)
            pulse_width = echo_end - us->echo_start;
        else
            pulse_width = (0x10000UL - us->echo_start) + echo_end; // Handle 16-bit rollover

        us->echo_width = pulse_width;
        us->distance = pulse_width / 58U;
        us->new_data_ready = 1;
    }
}
```

> **Why Previous Attempts Failed:**  
> In prior attempts (e.g. Mazen's solution), `HAL_GetTick()` was used to measure the echo pulse. Because `HAL_GetTick()` has a resolution of $1\text{ ms}$ ($1000\ \mu\text{s}$), any sub-millisecond echo pulse returned $0$, making the sensor appear completely unresponsive. Our solution uses a true $1\ \mu\text{s}$ hardware timer (`TIM1`), providing exact readings down to $1\text{ cm}$.

---

## 5. Periodic Pulse Waveform Generator

### Timing Specifications (from `Task3_STM.pdf`)

The waveform requires a repeating sequence of asymmetric pulses:

```
Voltage
 (3.3V) |          +----------+                +----+
        |          |          |                |    |
   (0V) +----------+          +----------------+    +--------------+
        |   10µs   |   20µs   |      10µs      | 5µs|     15µs     |
        0          10         30               40   45             60 (µs)
```

| Phase | Interval Range | Duration | Output Level | Output Voltage | Action in ISR |
| :---: | :---: | :---: | :---: | :---: | :--- |
| **0** | $0 \to 10\ \mu\text{s}$ | $10\ \mu\text{s}$ | **LOW** | $0\text{ V}$ | Set pins LOW, program $\text{ARR} = 20 - 1$ |
| **1** | $10 \to 30\ \mu\text{s}$ | $20\ \mu\text{s}$ | **HIGH** | $3.3\text{ V}$ | Set pins HIGH, program $\text{ARR} = 10 - 1$ |
| **2** | $30 \to 40\ \mu\text{s}$ | $10\ \mu\text{s}$ | **LOW** | $0\text{ V}$ | Set pins LOW, program $\text{ARR} = 5 - 1$ |
| **3** | $40 \to 45\ \mu\text{s}$ | $5\ \mu\text{s}$ | **HIGH** | $3.3\text{ V}$ | Set pins HIGH, program $\text{ARR} = 15 - 1$ |
| **4** | $45 \to 60\ \mu\text{s}$ | $15\ \mu\text{s}$ | **LOW** | $0\text{ V}$ | Set pins LOW, loop back to Phase 0 |

### Fast Atomic Pin Manipulation

To guarantee sub-microsecond edge precision without jitter, pins are driven directly via the **Bit Set/Reset Register** (`BSRR`):

```c
/* Turn HIGH atomically */
GPIOA->BSRR = (1U << 5); // PA5
GPIOB->BSRR = (1U << 8); // PB8

/* Turn LOW atomically */
GPIOA->BSRR = (1U << (5 + 16)); // PA5
GPIOB->BSRR = (1U << (8 + 16)); // PB8
```

---

## 6. LCD Display Formatting

The 16x2 LCD displays continuous real-time status:

```
+----------------+
|Dist:  45 cm    |  <-- Line 1: Live distance in cm (0x80)
|O-Scope: PA5/PB8|  <-- Line 2: Waveform output pin indicator (0xC0)
+----------------+
```

- When the target is out of detection range ($> 400\text{ cm}$ or $0\text{ cm}$), Line 1 cleanly displays: `Dist: OutRange`.
- Direct line overwriting (fixed 16-character string buffer) is used instead of screen clearing to prevent display flicker in Proteus.

---

## 7. How to Build & Simulate

### Compilation via Command Line

```powershell
# Run the automated build script
powershell -ExecutionPolicy Bypass -File scratch/build_task3.ps1
```

The build executes `arm-none-eabi-gcc` with `-O0`, producing:

- `Debug/STM32F103C8TX.elf`
- `Debug/STM32F103C8TX.hex`

### Running Simulation in Proteus

1. Open [`Task3-Proteus.pdsprj`](./STM32F103C8TX/STM32F103C8TX/Task3-Proteus.pdsprj) in **Proteus 8 Professional**.
2. The schematic includes:
   - **STM32F103C8** pre-configured to load `Debug\STM32F103C8TX.hex` with an $8\text{ MHz}$ internal clock.
   - **Ultrasonic Sensor (HC-SR04)** with test potentiometer to vary simulated distance.
   - **LM016L (16x2 LCD)** connected to `PA0`–`PA2` and `PB0`–`PB7`.
   - **Digital Oscilloscope** connected to `PA5` and `PB8`.
3. Press **Play** in Proteus:
   - Observe the distance updating on the LCD as you adjust the sensor pot.
   - Open the Oscilloscope window to observe the asymmetric $60\ \mu\text{s}$ periodic waveform.
