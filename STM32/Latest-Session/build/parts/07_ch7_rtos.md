# 7. Part 6 — Real-Time Operating Systems (RTOS) and FreeRTOS

As embedded systems expand from simple single-purpose controllers to complex multi-rate devices—integrating motor PID loops, sensor fusion algorithms, wireless communication stacks, file systems, and user interfaces—traditional bare-metal superloops begin to collapse under their own complexity. A blocking `HAL_Delay()` executed during a sensor read halts the entire processor, delaying critical motor adjustments or dropping incoming UART packets.

A **Real-Time Operating System (RTOS)** introduces a preemptive, priority-based multitasking scheduler that allows developers to decompose complex monolithic firmware into independent, concurrent execution threads (**Tasks**). 

This chapter examines the internal mechanics of embedded operating systems, focusing on **FreeRTOS** running on ARM Cortex-M microcontrollers. We analyze context-switching physics, scheduling algorithms, thread synchronization primitives, interrupt integration boundaries, and critical HAL timebase configuration rules.

---

## 7.1 Bare-Metal Superloops vs. RTOS: When is an RTOS Justified?

An RTOS is not a status symbol or an automatic badge of engineering quality. It is a tool for managing complex execution timing.

### 7.1.1 The Anatomy of a Bare-Metal Superloop
In a traditional bare-metal architecture, firmware executes within an infinite loop:

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    App_Init();

    while (1) {
        App_ProcessCommands();     /* Parses CLI packets */
        App_ReadSensors();         /* Samples I2C/SPI sensors */
        App_UpdateControlLoop();   /* Computes motor PID */
        App_SendTelemetry();       /* Streams UART data */
    }
}
```

The fundamental limitation of the superloop is that **all tasks share a single call stack and run sequentially**:
- If `App_ReadSensors()` takes $5\text{ ms}$ to complete an I²C read, `App_UpdateControlLoop()` is delayed by at least $5\text{ ms}$.
- If someone inserts a blocking `HAL_Delay(100)` inside a state machine, the entire system freezes for $100\text{ ms}$.
- Meeting strict deadlines requires restructuring the code into cooperative non-blocking state machines using `HAL_GetTick()` timestamps. Once a project exceeds 4 or 5 independent timing requirements, state-machine code becomes unmaintainable.

---

### 7.1.2 The RTOS Justification Matrix

**Table 7.1 — Superloop vs. RTOS selection criteria**

| Architecture | Fits When | Breaks Down When |
|---|---|---|
| **Bare-Metal Superloop** | • Simple, coupled activities with harmonic rates (e.g. 100 Hz and 10 Hz).<br>• Memory is severely constrained (< 4 kB RAM).<br>• Execution is purely event-driven using hardware interrupts.<br>• Functions are non-blocking and have bounded Worst-Case Execution Time (WCET). | • Multiple asynchronous, independent deadlines coexist.<br>• Third-party stacks (USB, TCP/IP, FATFS) require dedicated blocking loops.<br>• Developers insert blocking delays inside main logic.<br>• Adding a new feature breaks the timing of existing features. |
| **Real-Time OS (FreeRTOS)** | • Independent activities require strict priority-based preemption.<br>• Background logging or telemetry must not starve a 1 kHz motor control loop.<br>• Reusable software components communicate via structured queues.<br>• System uses complex middleware (USB host, cellular modem, filesystem). | • Memory is too small to afford private stacks per task.<br>• Developers believe an RTOS "makes slow code run faster".<br>• Engineers create dozens of tasks without budgeting RAM.<br>• Developers use `HAL_Delay()` inside high-priority tasks. |

**Table 7.2 — Good vs. poor reasons for adopting an RTOS**

| Valid Engineering Justifications | Flawed / Dangerous Misconceptions |
|---|---|
| Independent deadlines where a high-priority task must preempt long background transfers | *"Professionals always use an RTOS, so we should too."* |
| Integrating complex blocking middleware (e.g., USB device stack or LwIP Ethernet) | *"We will use an RTOS so we don't have to write non-blocking state machines."* |
| Clean separation of concerns with thread-safe inter-task queues | *"An RTOS will make our slow 100 kHz I²C read execute faster."* |
| Deterministic rate enforcement using `vTaskDelayUntil()` | *"We will create 25 tasks for every single function in the project."* |

---

## 7.2 Core RTOS Architecture and Mechanics

### 7.2.1 Tasks, Stacks, and the Task Control Block (TCB)

In FreeRTOS, a **Task** is an independent thread of execution represented as an infinite loop that never returns. Each task possesses:
1. **A Function Pointer**: The entry point of the task code.
2. **A Priority**: An integer from `0` (Idle task priority) up to `configMAX_PRIORITIES - 1`.
3. **A Private Stack**: A dedicated block of SRAM allocated from the FreeRTOS heap. The task stores its local variables, function call frames, and saved CPU registers here.
4. **A Task Control Block (TCB)**: A system data structure that records the task's state, current top of stack pointer (`pxTopOfStack`), priority, and list links.

```text
       SRAM Allocation per Task
┌──────────────────────────────────────┐
│ Task Control Block (TCB)             │
│ • pxTopOfStack                       │
│ • uxPriority                         │
│ • pcTaskName                         │
├──────────────────────────────────────┤
│ Task Private Stack                   │
│ (Local variables, nested calls,      │
│  and saved CPU registers R0-R15)     │
│                   ▲                  │
│                   │ Grows downwards  │
└──────────────────────────────────────┘
```

---

### 7.2.2 Preemptive Scheduling and Rate Monotonic Intuition

FreeRTOS implements a **Preemptive Priority-Based Scheduler with Round-Robin Time-Slicing**:
- **Strict Preemption**: The scheduler guarantees that the **highest-priority task in the READY state is ALWAYS running on the CPU**.
- If a low-priority telemetry task is executing and a high-priority motor task transitions from BLOCKED to READY (e.g., because a timer expired or an interrupt posted data), the scheduler **immediately preempts** the low-priority task.
- Tasks with equal priority share the CPU equally via Round-Robin time-slicing at every **SysTick tick interrupt** (typically configured to $1000\text{ Hz} \implies 1\text{ ms}$ tick).

> [!CAUTION]
> **Priority Starvation Hazard**: In a preemptive system, if a high-priority task enters an infinite computation loop without calling a blocking FreeRTOS API (`vTaskDelay`, `xQueueReceive`, `xSemaphoreTake`), **no lower-priority task will ever execute**. Even the FreeRTOS Idle Task (which frees dynamic memory and services power-saving modes) will starve!

#### Rate Monotonic Scheduling (RMS) Guideline
When assigning task priorities: **Tasks with shorter execution periods / tighter deadlines should be assigned higher numerical priorities**.
- Priority 4: Fast Motor PID / Inverter Control ($1000\text{ Hz}$, $1\text{ ms}$ period)
- Priority 3: IMU Sensor Reading & Attitude Filter ($200\text{ Hz}$, $5\text{ ms}$ period)
- Priority 2: Battery & Temperature Monitor ($1\text{ Hz}$, $1000\text{ ms}$ period)
- Priority 1: UART / Wireless Telemetry Stream ($10\text{ Hz}$, $100\text{ ms}$ period)

---

### 7.2.3 Context Switching on ARM Cortex-M: PendSV and SysTick

The transition from one task to another is called a **Context Switch**. On ARM Cortex-M processors (Cortex-M0/M3/M4/M7), this is executed using the **PendSV (Pended Servicing Call)** exception.

```text
 1. SysTick Interrupt fires (1 ms)
 2. Scheduler detects Task A (Priority 2) must yield to Task B (Priority 3)
 3. Scheduler sets PendSV interrupt pending bit in ICSR
 4. Active ISR concludes; CPU immediately enters PendSV_Handler (lowest priority)
 5. PendSV saves Task A's remaining registers (R4-R11) to Task A's stack
 6. PendSV switches Process Stack Pointer (PSP) to Task B's stack
 7. PendSV restores Task B's registers (R4-R11)
 8. PendSV executes Exception Return (0xFFFFFFFD); Hardware pops R0-R3, R12, LR, PC
 9. Task B resumes execution exactly where it was preempted!
```

#### Hardware Auto-Stacking vs. Software Stacking
The ARM Cortex-M architecture splits context saving into two phases:
1. **Hardware Automatic Stacking**: Upon entering any exception, the hardware automatically pushes 8 registers onto the current stack (**R0, R1, R2, R3, R12, LR, PC, and xPSR**).
2. **Software Stacking inside PendSV**: The FreeRTOS assembly handler pushes the remaining 8 core registers (**R4 through R11**) onto the task stack, saves the stack pointer into the task's TCB, loads the new task's stack pointer, pops its R4–R11 registers, and returns.

---

## 7.3 Inter-Task Communication and Synchronization Primitives

Tasks must exchange data and synchronize actions safely without race conditions.

**Table 7.3 — FreeRTOS synchronization and communication primitives**

| Primitive | Operating Mechanism | Typical Use Case | ISR Safe? |
|---|---|---|---|
| **Queue** | FIFO buffer; copies data **by value** into queue storage | Passing sensor samples from producer to consumer | Yes (`FromISR` variant) |
| **Binary Semaphore** | Flag with state 0 or 1; unblocks a waiter | Task-to-task or ISR-to-task event signaling | Yes (`FromISR` variant) |
| **Counting Semaphore** | Integer token counter (0 to $N$) | Managing a pool of identical resources; event counting | Yes (`FromISR` variant) |
| **Mutex** | Mutual exclusion lock with **Priority Inheritance** | Protecting a shared hardware bus (e.g. single I²C bus) | **NO! Never call from an ISR** |
| **Task Notification** | Direct 32-bit value updated inside target task's TCB | Lightweight, zero-RAM event signaling (45% faster) | Yes (`FromISR` variant) |
| **Event Group** | Bitmask of flags; tasks wait for pattern (AND/OR) | Synchronizing complex multi-stage boot sequences | Yes (`FromISR` variant) |

---

### 7.3.1 Priority Inversion and Priority Inheritance in Mutexes

The **Priority Inversion Problem** is a classic real-time failure mode that famously affected the NASA Mars Pathfinder spacecraft:

```text
High Priority Task H:   ───[Wants Bus]───► BLOCKED by Mutex! ─────────────► Resumes
                                            ▲
Medium Priority Task M: ────────────────────┼─────[Preempts Task L]──────►
                                            │
Low Priority Task L:    ──[Takes Mutex]─────┴──────────────────[Releases]─►
```

1. Low-priority Task $L$ acquires a mutex protecting a shared I²C bus.
2. High-priority Task $H$ wakes up and attempts to take the mutex. Seeing it locked, Task $H$ blocks.
3. Medium-priority Task $M$ (which does not even use the I²C bus) wakes up. Because Task $M$ has higher priority than Task $L$, **Task $M$ preempts Task $L$**!
4. Task $L$ cannot finish its transaction to release the mutex.
5. As a result, **the highest-priority Task $H$ is indirectly blocked by the medium-priority Task $M$ for an unbounded duration**!

#### The Solution: Priority Inheritance
FreeRTOS **Mutexes** (unlike basic binary semaphores) feature automatic **Priority Inheritance**:
- When Task $H$ blocks waiting for a mutex held by Task $L$, the scheduler **temporarily elevates Task $L$'s priority to match Task $H$**.
- Medium-priority Task $M$ can no longer preempt Task $L$.
- Task $L$ quickly finishes its bus transaction, releases the mutex, and is immediately demoted back to its low priority, allowing Task $H$ to proceed without delay.

---

## 7.4 Interrupts and FreeRTOS: The Critical Priority Rule

Interfacing hardware interrupts with FreeRTOS is the most common source of HardFaults and system freezes among STM32 developers.

### 7.4.1 Strict ISR API Rules
1. **Never call standard FreeRTOS APIs inside an ISR**. Always use the `FromISR` variants (e.g., `xQueueSendFromISR()`, `xSemaphoreGiveFromISR()`).
2. **Never attempt to take a Mutex inside an ISR**. Mutexes can block; an interrupt service routine cannot block.
3. **Never call `vTaskDelay()` inside an ISR**.

---

### 7.4.2 The `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` Rule

ARM Cortex-M NVIC allows assigning numerical priorities to interrupts, where **$0$ is the highest physical urgency and $15$ is the lowest** (on 4-bit priority implementations).

In `FreeRTOSConfig.h`, two vital priority thresholds are configured:
- `configLIBRARY_LOWEST_INTERRUPT_PRIORITY`: Typically set to `15` (lowest urgency; assigned to SysTick and PendSV).
- `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`: Typically set to `5`.

```text
Numerical Priority:
 0 (Highest Urgency) ──┐
 1                     │ Zero-Latency Interrupts:
 2                     │ CANNOT call any FreeRTOS API!
 3                     │ Never masked by critical sections.
 4 ────────────────────┘
 5 (configMAX_SYSCALL) ──┐
 6                       │ FreeRTOS-Aware Interrupts:
 7                       │ CAN call *FromISR APIs.
 ...                     │ Masked during FreeRTOS critical sections.
15 (Lowest Urgency)  ───┘
```

> [!CAUTION]
> **The FreeRTOS NVIC Priority Law**: Any interrupt service routine that calls a FreeRTOS `FromISR` API **MUST have its NVIC priority set numerically equal to or higher than `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`** (i.e., a priority value between $5$ and $15$).
> 
> If an ISR with NVIC priority $0, 1, 2, 3,$ or $4$ calls `xQueueSendFromISR()`, it corrupts the internal FreeRTOS scheduler lists, triggering an immediate assertion failure in `vPortValidateInterruptPriority()` or an unrecoverable HardFault!
> 
> Furthermore, in CubeMX, you **MUST configure NVIC Priority Grouping to `NVIC_PRIORITYGROUP_4`** (4 bits preemption, 0 bits subpriority). Any other grouping violates FreeRTOS priority calculation assumptions.

---

## 7.5 Production Example: Thread-Safe Multi-Task Architecture

The following complete, verified FreeRTOS implementation demonstrates a dual-task architecture:
1. **Sensor Task ($50\text{ Hz}$)**: Samples the MPU6050 with fixed period, packs readings into a `SensorSample` struct, and pushes to a queue.
2. **Telemetry Task (Event-Driven)**: Blocks waiting on the queue and streams ASCII packets over UART.
3. **Shared I²C Bus Protection**: Guarded by a dedicated Mutex.

**Listing 7.1 — Production FreeRTOS dual-task telemetry architecture**

```c
#include "main.h"
#include "cmsis_os.h"
#include "mpu6050.h"
#include <stdio.h>
#include <string.h>

/* Data sample packet passed through the queue */
typedef struct {
    uint32_t timestamp_ms;
    int16_t  accel_x_mg;      /* Fixed-point integer (1000 = 1.0g) */
    int16_t  accel_y_mg;
    int16_t  accel_z_mg;
    int16_t  gyro_z_deg_x10;  /* 125 = 12.5 deg/sec */
} SensorPacket_t;

/* FreeRTOS Handles */
QueueHandle_t     xSensorQueue;
SemaphoreHandle_t xI2CMutex;

/* Task Handles */
TaskHandle_t xSensorTaskHandle;
TaskHandle_t xTelemetryTaskHandle;

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
extern mpu6050_t imu;

/* =========================================================================
   TASK 1: Sensor Acquisition Task (High Priority: 3, Period: 20 ms / 50 Hz)
   ========================================================================= */
void SensorTask(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(20); /* Strict 20 ms period */
    mpu6050_data_t imu_data;
    SensorPacket_t packet;

    for (;;) {
        /* Deterministic periodic execution */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        /* Acquire I2C Bus Mutex with 10 ms timeout */
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            mpu6050_status_t status = MPU6050_ReadSensors(&imu, &imu_data);
            xSemaphoreGive(xI2CMutex); /* Release bus immediately */

            if (status == MPU6050_OK) {
                packet.timestamp_ms    = HAL_GetTick();
                packet.accel_x_mg      = (int16_t)(imu_data.accel_x_mps2 * 101.97f);
                packet.accel_y_mg      = (int16_t)(imu_data.accel_y_mps2 * 101.97f);
                packet.accel_z_mg      = (int16_t)(imu_data.accel_z_mps2 * 101.97f);
                packet.gyro_z_deg_x10  = (int16_t)(imu_data.gyro_z_dps * 10.0f);

                /* Post to queue with 5 ms timeout; drop sample if queue is full */
                if (xQueueSend(xSensorQueue, &packet, pdMS_TO_TICKS(5)) != pdPASS) {
                    /* Queue overflow policy: drop sample and record diagnostic counter */
                }
            }
        }
    }
}

/* =========================================================================
   TASK 2: Telemetry Stream Task (Medium Priority: 2, Event-Driven)
   ========================================================================= */
void TelemetryTask(void *argument)
{
    SensorPacket_t packet;
    char uart_msg[96];

    for (;;) {
        /* Block indefinitely until a sensor packet arrives in the queue */
        if (xQueueReceive(xSensorQueue, &packet, portMAX_DELAY) == pdTRUE) {
            int len = snprintf(uart_msg, sizeof(uart_msg),
                              "$IMU,%lu,%d,%d,%d,%d\r\n",
                              packet.timestamp_ms,
                              packet.accel_x_mg, packet.accel_y_mg, packet.accel_z_mg,
                              packet.gyro_z_deg_x10);

            if (len > 0) {
                HAL_UART_Transmit(&huart2, (uint8_t *)uart_msg, (uint16_t)len, 50);
            }
        }
    }
}

/* =========================================================================
   Application Initialization Hook
   ========================================================================= */
void App_RTOS_Init(void)
{
    /* Create Queue for 16 packets (fixed allocation) */
    xSensorQueue = xQueueCreate(16, sizeof(SensorPacket_t));
    xI2CMutex    = xSemaphoreCreateMutex();

    /* Create Tasks with explicit stack budgets (words, where 1 word = 4 bytes) */
    xTaskCreate(SensorTask, "SensorTask", 256, NULL, 3, &xSensorTaskHandle);
    xTaskCreate(TelemetryTask, "TelemTask", 384, NULL, 2, &xTelemetryTaskHandle);

    /* Start FreeRTOS Scheduler (never returns) */
    vTaskStartScheduler();
}
```

---

## 7.6 Critical CubeMX Setup: The HAL Timebase Source Conflict

When FreeRTOS is enabled in an STM32CubeMX project, **both FreeRTOS and the STM32 HAL library require a 1 ms periodic tick interrupt**:
- FreeRTOS uses the Cortex-M **SysTick** timer to manage task time-slicing and unblock delays.
- HAL functions (`HAL_Delay()`, `HAL_GetTick()`, internal driver timeouts) also depend on a 1 ms tick.

If both systems share SysTick, a severe priority conflict arises: FreeRTOS configures SysTick at the lowest interrupt priority (`15`) so that hardware ISRs can preempt task switching. If a hardware interrupt handler calls a HAL function with a timeout (such as `HAL_I2C_Mem_Read()`), the SysTick interrupt cannot preempt the ISR. As a result, `HAL_GetTick()` stops incrementing, **and the HAL function hangs indefinitely inside an infinite loop!**

### The Required CubeMX Configuration
STMicroelectronics officially requires separating the HAL timebase from FreeRTOS:
1. In CubeMX, open the **System Core → SYS** category.
2. Locate the **Timebase Source** dropdown (which defaults to *SysTick*).
3. **Change Timebase Source to an unused general-purpose timer** (e.g., **TIM4** on STM32F103, or **TIM6/TIM7** on STM32F4/G4).
4. CubeMX will automatically generate `TIM4_IRQHandler()` to drive `HAL_IncTick()`, leaving SysTick exclusively dedicated to FreeRTOS.

```text
CubeMX SYS Configuration:
┌────────────────────────────────────────────────────────┐
│ System Core → SYS                                      │
│ • Debug: Serial Wire                                   │
│ • Timebase Source: TIM4  ◄── DO NOT LEAVE AS SysTick! │
└────────────────────────────────────────────────────────┘
```
