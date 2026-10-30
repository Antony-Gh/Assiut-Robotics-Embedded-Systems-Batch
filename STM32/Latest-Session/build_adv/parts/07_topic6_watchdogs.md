# 7. Topic 6 — Watchdog Timers: IWDG and WWDG

In hobbyist coding, when software crashes or deadlocks inside an infinite loop, the developer simply presses the physical reset button or power-cycles the board.

In professional embedded engineering and autonomous robotics, **this luxury does not exist**. A drone flying 50 meters above ground, an autonomous mobile robot navigating an industrial warehouse, or an unattended environmental sensor station in the desert cannot be manually reset.

Firmware can lock up due to unpredictable real-world factors:
1. **Electromagnetic Interference (EMI)** from motor commutation arcing that corrupts the CPU Program Counter (`PC`).
2. **I²C / SPI Bus Deadlocks** where a sensor asserts continuous clock stretching or fails to respond, trapping firmware inside an unbounded `while(!flag)` loop.
3. **FreeRTOS Task Priority Inversions or Deadlocks** where two threads compete indefinitely for mutual exclusion locks.
4. **Stack Overflow Corruptions** that overwrite return pointers on the call stack.

To guarantee that an embedded system can **recover autonomously from any firmware freeze**, every STM32 microcontroller includes hardware **Watchdog Timers**.

This chapter details the architecture and mathematical timing of the **Independent Watchdog (IWDG)** and **Window Watchdog (WWDG)**, proves why blindly refreshing a watchdog inside a timer ISR destroys system reliability, implements a production-grade multi-task FreeRTOS watchdog supervisor, and demonstrates how to freeze watchdogs during GDB debugging.

![Figure 6.1 — Watchdog timing windows and FreeRTOS multi-task supervision architecture](diagrams_adv/fig6_watchdog_architectures.png)
*Figure 6.1 — (A) Timing window comparison: IWDG permissible refresh range vs. WWDG early and late reset zones; (B) FreeRTOS multi-task watchdog supervisor pattern utilizing event groups to verify thread health before petting.*

---

## 7.1 Watchdog Architecture: Hardware vs. Software Watchdogs

A **Watchdog Timer** is an autonomous hardware countdown counter. When enabled:
1. The hardware counter continuously decrements towards zero at a fixed clock rate.
2. Firmware is responsible for periodically "refreshing" (or "petting/kicking") the watchdog by writing a specific reload key into its hardware register, which resets the counter back to its maximum value.
3. If firmware hangs, deadlocks, or crashes, it fails to reload the counter.
4. When the counter reaches zero (or underflows), the watchdog hardware asserts an electrical **System Reset (`NRST`)**, rebooting the microcontroller back to a known clean state.

### Why Software-Only Watchdogs Are Insufficient
A software watchdog implemented as a timer variable inside firmware is vulnerable: if the CPU crashes, memory is corrupted, or interrupts are masked, the software variable stops evaluating.
The STM32 watchdogs are **true silicon hardware peripherals physically independent of the ARM core**.

---

## 7.2 The Independent Watchdog (IWDG)

The **Independent Watchdog (IWDG)** is the primary safety watchdog in STM32 microcontrollers.

### 7.2.1 The Dedicated LSI Clock Domain
The defining architectural feature of the IWDG is that it is clocked by its own **dedicated on-chip Low-Speed Internal (LSI) RC oscillator ($f_{\text{LSI}} \approx 40\text{ kHz}$ on STM32F1, $\approx 32\text{ kHz}$ on STM32F4)**:
- **Total Independence**: The LSI runs entirely isolated from the main high-speed system oscillators (HSE, HSI, and PLL).
- Even if the main external crystal fractures, the PLL loses lock, or the primary bus clock stops completely, **the IWDG continues counting down in silicon**, guaranteeing an autonomous hardware reboot!

---

### 7.2.2 Register Interface and Key Security
To prevent errant pointers or stack corruption from accidentally writing to the watchdog registers, access is guarded by the **Key Register (`IWDG_KR`)**:
- Write `0x5555`: Unlocks write access to the Prescaler (`IWDG_PR`) and Reload (`IWDG_RLR`) registers.
- Write `0xAAAA`: Reloads the counter with the value in `IWDG_RLR` ("pets" the watchdog).
- Write `0xCCCC`: **Starts the IWDG**.

> [!CAUTION]
> **The Irreversible Hardware Rule**: Once the IWDG is started by writing `0xCCCC` (or via CubeMX initialization), **IT CANNOT BE STOPPED BY SOFTWARE**. It will count down until it is refreshed or until it resets the chip. The only way to stop an active IWDG is a hardware power-down or system reset.

---

### 7.2.3 Timeout Calculations and Mathematical Formulas

The IWDG timeout duration ($T_{\text{IWDG}}$) is governed by the LSI frequency ($f_{\text{LSI}}$), the prescaler divider ($P$), and the 12-bit reload value ($\text{RLR} \in [0, 4095]$):

$$T_{\text{IWDG}} = \frac{P \times (\text{RLR} + 1)}{f_{\text{LSI}}}$$

Where the prescaler $P$ is determined by bits `PR[2:0]` in `IWDG_PR`:

$$P = 4 \times 2^{\text{PR\_bits}} \in \{4, 8, 16, 32, 64, 128, 256\}$$

#### Minimum and Maximum Timeouts ($f_{\text{LSI}} = 40\text{ kHz}$ on STM32F103)
- **Minimum Timeout** ($P = 4, \text{RLR} = 0$):
  $$T_{\min} = \frac{4 \times 1}{40{,}000} = 0.0001\text{ s} = \mathbf{0.1\text{ ms}}$$
- **Maximum Timeout** ($P = 256, \text{RLR} = 4095$):
  $$T_{\max} = \frac{256 \times 4096}{40{,}000} = \frac{1{,}048{,}576}{40{,}000} \approx \mathbf{26.21\text{ seconds}}$$

#### Worked Engineering Calculation: Designing a $500\text{ ms}$ Safety Watchdog
Target timeout: $T_{\text{target}} = 500\text{ ms} = 0.500\text{ s}$.
1. Choose Prescaler $P = 64$ (`IWDG_PRESCALER_64`).
   $$\text{Tick Period} = \frac{64}{40{,}000\text{ Hz}} = 0.0016\text{ s} = 1.6\text{ ms per tick}$$
2. Calculate Reload Register (`RLR`):
   $$\text{RLR} + 1 = \frac{T_{\text{target}}}{\text{Tick Period}} = \frac{500\text{ ms}}{1.6\text{ ms}} = 312.5 \implies \mathbf{\text{RLR} = 312}$$
3. Actual verified timeout:
   $$T_{\text{actual}} = \frac{64 \times (312 + 1)}{40{,}000} = \frac{20{,}032}{40{,}000} = \mathbf{500.8\text{ ms}}$$

---

### 7.2.4 Verified HAL Configuration

**Listing 7.1 — Initializing and refreshing the IWDG in STM32 HAL**

```c
#include "main.h"

IWDG_HandleTypeDef hiwdg;

void MX_IWDG_Init(void)
{
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64; /* P = 64 */
    hiwdg.Init.Reload    = 312;                /* 500 ms timeout */

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        /* Initialization Error */
        Error_Handler();
    }
}

/* Call periodically in application code before 500 ms expires */
void App_KickWatchdog(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}
```

---

## 7.3 The Window Watchdog (WWDG)

While the IWDG catches firmware that fails to execute, the **Window Watchdog (WWDG)** catches subtle execution failures where firmware runs **TOO FAST**—such as being trapped in a tight, unthrottled spin-loop or skipping vital validation steps.

### 7.3.1 Operating Principle: The Window Concept
The WWDG is clocked directly from the internal **APB1 peripheral bus** (divided by a fixed prescaler of 4096).
It features a **7-bit downcounter (`T[6:0]`)** and a configurable **7-bit Window Register (`W[6:0]`)**:

```text
Counter State T[6:0]:
 0x7F ───┐
         │ FORBIDDEN EARLY ZONE: Refreshing here triggers an IMMEDIATE RESET!
 W ──────┴─────────────────────────────────────────────────────────────────
         │
         │ PERMITTED REFRESH WINDOW: Refresh allowed only when 0x3F < T <= W!
         │
 0x40 ───┬─────────────────────────────────────────────────────────────────
         │ LATE TIMEOUT ZONE: If T decrements to 0x3F, hardware asserts RESET!
 0x3F ───┘
```

#### The Two Reset Trigger Conditions
1. **Late Refresh (Underflow)**: If the counter decrements from `0x40` to `0x3F` (bit 6 clears), hardware immediately asserts a system reset.
2. **Early Refresh (Window Violation)**: If firmware attempts to reload the counter while `T > W` (in the upper forbidden zone), **hardware triggers an immediate system reset**!

---

### 7.3.2 The Early Wakeup Interrupt (EWI)
The WWDG includes an emergency interrupt: the **Early Wakeup Interrupt (EWI)**.
When enabled, the EWI fires the exact moment the counter reaches `0x40` (one tick before the system resets at `0x3F`).
- *Engineering Purpose*: Provides firmware with a final window of $\sim 50\ \mu\text{s}$ to execute emergency failsafe procedures before the CPU reboots:
  - Immediately disable motor PWM outputs to prevent runaway robotics movement.
  - Flush a critical error diagnostic code into non-volatile Backup registers.

---

### 7.3.3 IWDG vs. WWDG: Architectural Trade-Offs

**Table 7.1 — Comprehensive comparison: IWDG vs. WWDG**

| Feature | Independent Watchdog (IWDG) | Window Watchdog (WWDG) |
|---|---|---|
| **Clock Source** | Dedicated independent LSI RC oscillator ($40\text{ kHz}$) | Main APB1 peripheral bus clock |
| **Clock Reliability** | **Survives main clock and crystal failure** | Stops if APB1 clock fails |
| **Refresh Constraint** | Refresh allowed at **ANY** time before timeout | **Must refresh strictly inside $[0x40, W]$ window** |
| **Catches Runaway Loops?** | No (Only detects hanging / stalled code) | **Yes (Catches code running abnormally fast)** |
| **Interrupt Capability** | None (Immediate reset upon timeout) | Early Wakeup Interrupt (`EWI`) supported |
| **Typical Use Case** | Industrial robotics safety; deadlocks; failsafe reboot | Medical equipment, avionics, motor control timing |

---

## 7.4 Watchdogs in Production: Where Should You Refresh?

The single most common mistake in embedded watchdog implementation is refreshing the watchdog in the wrong place.

> [!CAUTION]
> **The Timer ISR Rookie Trap**: Novice engineers frequently put `HAL_IWDG_Refresh()` inside a periodic timer interrupt (e.g. `SysTick_Handler()` or a 100 Hz timer callback).
> 
> **THIS COMPLETELY DESTROYS WATCHDOG PROTECTION!**
> 
> In ARM Cortex-M microcontrollers, hardware interrupts execute independently of the `main()` loop. If your `main()` task deadlocks, if memory is corrupted, or if FreeRTOS locks up, **the timer interrupt will continue firing and happily petting the watchdog forever**! The watchdog will never trigger, and your frozen robot will never recover.

### 7.4.1 Bare-Metal Rule
In a bare-metal superloop:
- **Never refresh inside an ISR**.
- Refresh strictly at the end of the `main()` loop **after verifying that every subsystem state machine returned `STATUS_OK`**:

```c
while (1) {
    if (ReadSensors() == OK && UpdatePID() == OK && SendTelemetry() == OK) {
        HAL_IWDG_Refresh(&hiwdg); /* Pet ONLY if all subsystems healthy */
    }
}
```

---

### 7.4.2 The FreeRTOS Multi-Task Watchdog Supervisor Pattern

In a multi-threaded FreeRTOS application, multiple tasks execute concurrently at different priorities. If one low-priority telemetry task deadlocks, a simple superloop refresh does not exist.

The professional architectural solution is the **Watchdog Daemon Task with Event Group Supervision**:

```text
 [Sensor Task (50 Hz)] ──► Sets Bit 0x01
 [Motor Task (200 Hz)] ──► Sets Bit 0x02 ──► FreeRTOS Event Group ──► [Watchdog Supervisor Task]
 [Comms Task (10 Hz)]  ──► Sets Bit 0x04       (0x07 = All Healthy)        │
                                                                           ▼
                                                                  HAL_IWDG_Refresh(&hiwdg)
```

1. Create a dedicated `WatchdogTask` running at medium priority.
2. Allocate an `EventGroupHandle_t xWatchdogEventGroup`.
3. Assign each critical task an individual bitmask flag:
   - Task 1 (Sensors): Bit `0x01`
   - Task 2 (Motor PID): Bit `0x02`
   - Task 3 (Telemetry): Bit `0x04`
4. Each task must explicitly set its bit upon completing its periodic cycle.
5. `WatchdogTask` wakes up every $300\text{ ms}$ and calls:
   ```c
   EventBits_t bits = xEventGroupWaitBits(xWatchdogEventGroup, 0x07, pdTRUE, pdTRUE, pdMS_TO_TICKS(400));
   ```
6. **If ALL bits (`0x07`) are set**: `WatchdogTask` calls `HAL_IWDG_Refresh(&hiwdg)`.
7. **If ANY task hangs or deadlocks**: its bit is missing. The Event Group times out, `WatchdogTask` **refuses to pet the IWDG**, and hardware resets the MCU automatically!

---

## 7.5 Debugging with Watchdogs: Freezing Counters via `DBGMCU`

When debugging firmware in STM32CubeIDE using an ST-LINK or J-Link probe, pausing execution at a breakpoint halts the CPU core.
However, by default, **the IWDG hardware counter keeps running in silicon**! Within 500 ms of hitting a breakpoint, the watchdog resets the microcontroller, abruptly disconnecting your debugger session.

### The Solution: DBGMCU Watchdog Freeze Registers
The STM32 Debug MCU (`DBGMCU`) peripheral includes dedicated control bits to freeze peripheral timers whenever the core is halted at a breakpoint.

**Listing 7.2 — Freezing watchdogs during debug sessions**

```c
void App_DebugConfig(void)
{
#if defined(DEBUG)
    /* Freeze IWDG and WWDG while halted in GDB / Breakpoint */
    #if defined(STM32F1)
        __HAL_DBGMCU_FREEZE_IWDG();
        __HAL_DBGMCU_FREEZE_WWDG();
    #elif defined(STM32F4)
        __HAL_DBGMCU_FREEZE_IWDG();
        __HAL_DBGMCU_FREEZE_WWDG();
    #endif
#endif
}
```
With this call placed in `main()` during initialization, the watchdog counter pauses whenever GDB halts the CPU, allowing unlimited inspection of variables, and resumes automatically when you click "Resume".
