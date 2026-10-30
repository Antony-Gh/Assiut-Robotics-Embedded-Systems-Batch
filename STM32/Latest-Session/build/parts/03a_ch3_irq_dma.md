# 3. Part 2 — Interrupts, DMA and Timers

This part answers three connected questions: *how does the CPU learn that something happened* (polling, interrupts), *how does data get into memory without the CPU copying it* (DMA), and *how does the hardware keep time and generate or measure signals* (timers). Section 3.7 combines all three into the backbone of real-time data acquisition.

## 3.1 Polling

**Definition.** In polling the CPU repeatedly reads a peripheral status flag (or calls a function that does so) until the event it is waiting for has occurred.

**How it works on STM32.** A HAL polling call such as `HAL_UART_Receive()` or `HAL_ADC_PollForConversion()` loops on a status bit (RXNE, EOC) inside the HAL, compares `HAL_GetTick()` against a timeout and returns `HAL_OK`, `HAL_TIMEOUT` or `HAL_ERROR`. While it waits, nothing else in that thread runs.

**Listing 3.1 — Polled ADC read with timeout**

```c
uint32_t raw = 0;
if (HAL_ADC_Start(&hadc1) == HAL_OK)
{
    if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
    {
        raw = HAL_ADC_GetValue(&hadc1);   /* 0..4095 for 12-bit */
    }
    (void)HAL_ADC_Stop(&hadc1);
}
```

*Strengths:* trivial to write and reason about, no interrupt or DMA configuration, deterministic when the awaited event is fast and bounded. *Weaknesses:* burns CPU cycles, blocks other work, timing depends on how much other work the loop does, and a flag that never sets locks the system unless a timeout exists.

**Use polling for:** initialization checks (`WHO_AM_I`), infrequent sensor reads (1 Hz temperature), short bounded operations, labs and prototypes. **Avoid it for:** continuous high-rate streams or anything sharing a thread with a hard-deadline loop.

## 3.2 Interrupts

### 3.2.1 Definition and purpose

An **interrupt** is a hardware-signaled request that makes the CPU suspend the current code, run a short **interrupt service routine (ISR)**, and then resume exactly where it left off. It exists so that the CPU can do useful work (or sleep) and still react within microseconds to events: a button edge, a timer update, a received byte, a conversion or DMA completion, a fault.

### 3.2.2 Architecture on Cortex-M

- **Exceptions and IRQs.** The core defines system exceptions (Reset, NMI, HardFault, SVCall, PendSV, SysTick) and the device adds **external interrupts (IRQn)** from peripherals, managed by the **NVIC** (Nested Vectored Interrupt Controller).
- **Vector table.** A table in Flash (symbol `g_pfnVectors` in `startup_*.s`) holds the initial stack pointer followed by the address of every handler. Entry for IRQ number *n* is at word index *16 + n*.
- **NVIC registers.** Per interrupt: enable (ISER/ICER), pending (ISPR/ICPR), active status and an 8-bit priority field of which the top 4 bits are implemented on most STM32 devices (16 levels).
- **Peripheral side.** Each peripheral has an interrupt-enable bit (for example `TIM_DIER.UIE`, `USART_CR1.RXNEIE`) and a status flag. The line to the NVIC is asserted only when both the enable and the flag are set; EXTI lines additionally filter GPIO edges.

![Figure 3.1 — Interrupt execution flow and DMA transfer architecture](diagrams/fig7_interrupt_dma_architecture.png)

*Figure 3.1 — (A) Interrupt execution flow from peripheral event to exception return; (B) DMA controller as a second bus master moving data between a peripheral register and SRAM.*

**Entry sequence.** When an enabled, pending interrupt has a higher priority than the running context, the core automatically pushes R0–R3, R12, LR, PC and xPSR (8 words) onto the active stack, fetches the vector, and starts the handler, about 12 cycles on Cortex-M3/M4 with zero-wait-state memory. If another interrupt is already pending at exit, **tail-chaining** skips the unstack/restack (about 6 cycles). Latency becomes much worse if Flash wait states, a higher-priority ISR or a long critical section intervene.

### 3.2.3 From IRQ to callback in HAL

CubeMX generates a one-line handler in `stm32f1xx_it.c`, and the HAL does the rest:

```c
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);         /* reads flags, clears them, dispatches */
}
```

`HAL_TIM_IRQHandler()` checks which flag fired, clears it and calls a **callback** such as `HAL_TIM_PeriodElapsedCallback()`. HAL callbacks are declared `__weak`; you provide your own definition. **Callbacks execute in interrupt context**, so all ISR rules apply to them. Forgetting to call the HAL handler from the IRQ function, or filtering the wrong `htim->Instance` inside the callback, are classic reasons a callback never runs.

### 3.2.4 Priorities, preemption and nesting

Cortex-M priorities are **inverted**: a *lower number is more urgent* (0 highest; 15 lowest with 4 implemented bits). The priority byte is split by the **priority grouping** into *preemption priority* and *sub-priority*. HAL sets `NVIC_PRIORITYGROUP_4` (4 bits preemption, 0 bits sub), which is also what FreeRTOS requires.

- A pending interrupt **preempts** the running ISR only if its *preemption* priority is numerically lower.
- Equal preemption priority never nests; the sub-priority only orders interrupts that are pending simultaneously.
- Configure with `HAL_NVIC_SetPriority(TIM3_IRQn, 5, 0); HAL_NVIC_EnableIRQ(TIM3_IRQn);` or in the CubeMX NVIC tab.
- Reserve the highest urgencies for the shortest, most time-critical handlers (motor fault, encoder), put communication in the middle and let the HAL tick and RTOS-aware interrupts sit lower.

**Listing 3.2 — Debounced button using EXTI**

```c
static volatile uint32_t g_last_press_ms;
static volatile uint8_t  g_button_event;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_Pin)                      /* label set in CubeMX */
    {
        uint32_t now = HAL_GetTick();
        if ((now - g_last_press_ms) > 30U)       /* 30 ms debounce window */
        {
            g_last_press_ms = now;
            g_button_event  = 1U;                /* main loop consumes this */
        }
    }
}
```

Newer HAL versions (for example STM32U5) split this callback into rising/falling variants; **Verify** the callback names in your package.

### 3.2.5 Strict ISR rules

> [!CAUTION]
> **ISR rules.**
>
> 1. Never block: no `HAL_Delay()`, `printf()`, `malloc()` or a `HAL_I2C_Master_Transmit()` with a long timeout. (`HAL_Delay()` in an ISR can deadlock if SysTick has lower priority than that ISR.)
> 2. Keep it to microseconds: set a flag, copy one byte into a ring buffer, or post to an RTOS queue/notification using the `...FromISR()` variant.
> 3. Make sure the interrupt source is cleared. A flag left set re-enters the ISR forever.
> 4. Share data deliberately. `volatile` forces the compiler to re-read a variable but does **not** make `counter++` atomic, protect structures, order accesses against DMA or provide cache coherency on Cortex-M7. For a flag between an ISR and `main` on Cortex-M3/M4 a `volatile uint8_t` load/store is sufficient; for 64-bit values or structs, disable interrupts briefly or use a lock-free index scheme.

**Latency budget.** Worst-case latency = longest higher-or-equal-priority ISR + longest interrupt-disabled section + Flash wait-state effects + entry overhead. A 200 µs ISR elsewhere will break a 10 µs SPI chip-select timing requirement. Measure with a scope by toggling a pin on entry and exit.

## 3.3 DMA — Direct Memory Access

### 3.3.1 Definition and purpose

A **DMA controller** is an autonomous second bus master that copies data between **endpoints** (peripheral register ↔ memory, or memory ↔ memory) according to a configuration you set once. It exists to remove the per-item cost of interrupts and polling from the CPU: at high data rates the *copying* would otherwise consume most of the CPU.

DMA moves bytes, half-words or words. It does **not** interpret data, filter noise, parse protocols or know what a sample means; that remains application work.

### 3.3.2 What actually happens during a transfer

1. **Setup (CPU, once).** The CPU programs source address, destination address, number of data items, item width, increment behavior, direction, mode (normal or circular) and priority, then enables the stream/channel.
2. **Peripheral request.** The peripheral raises a DMA request when it has data or needs data, for example ADC end-of-conversion (EOC), USART RXNE or TXE, SPI RXNE/TXE, timer update. (Memory-to-memory transfers have no peripheral request and run as fast as the bus allows.)
3. **Bus transaction (DMA).** The controller arbitrates for the bus matrix, reads one item from the source and writes it to the destination. Peripheral addresses are normally **fixed** (the data register); memory addresses **increment**.
4. **Bookkeeping.** The remaining-item counter decrements. At the halfway point the **half-transfer (HT)** flag sets; at zero the **transfer-complete (TC)** flag sets; a bus or configuration error sets the **transfer-error (TE)** flag. Each can raise an interrupt.
5. **Mode behavior.** In *normal mode* the channel disables itself at TC. In *circular mode* the counter and addresses reload and it continues forever.
6. **Notification (CPU).** HAL routes the DMA interrupt to a callback (`HAL_ADC_ConvHalfCpltCallback`, `HAL_UART_TxCpltCallback`, …) so that software can process or release the buffer.

The CPU therefore still **configures** DMA, **owns the buffer lifetime**, **handles completion and errors**, and **processes** the data. What it no longer does is touch each item.

### 3.3.3 Controller structure and configuration parameters

**Table 3.1 — DMA architecture by family**

| Family | Structure | Request routing | Practical consequence |
| --- | --- | --- | --- |
| F1 | Channels (DMA1: 7, DMA2: 5 on larger parts) | Fixed: each peripheral request is wired to specific channels | Check the reference-manual request table; two peripherals sharing a channel cannot run DMA simultaneously |
| F4 / F7 | Streams with channel selection, FIFO and burst options | Stream + channel number pairs | Easy to mis-click in CubeMX; the wrong stream means “callback never fires” |
| G0 / G4 / L4 / H7 / U5 | Channels or streams plus **DMAMUX** | Any request routed to any channel | Flexible; still must be enabled and mapped |

**Table 3.2 — Per-channel configuration fields (CubeMX DMA settings)**

| Field | Meaning | Typical ADC → RAM | Typical UART TX (RAM → peripheral) |
| --- | --- | --- | --- |
| Request / channel / stream | Which peripheral event triggers items | ADC1 | USARTx_TX |
| Direction | Peripheral→memory, memory→peripheral, memory→memory | Peripheral to memory | Memory to peripheral |
| Peripheral increment | Advance peripheral address each item | **Disabled** | **Disabled** |
| Memory increment | Advance memory address each item | **Enabled** | **Enabled** |
| Data width (periph / mem) | Byte, half-word, word | Half-word / half-word (12-bit in `uint16_t[]`) | Byte / byte |
| Mode | Normal or circular | Circular for continuous acquisition | Normal |
| Priority | Arbitration among DMA channels | Medium/high | Low/medium |
| FIFO / burst (F4/F7/H7) | Packing and burst bus use | Usually direct mode | Usually direct mode |

**Reference F103 mappings** (typical, **Verify** in RM0008 for your exact part): ADC1 → DMA1 channel 1; SPI1 RX/TX → DMA1 channel 2/3; USART1 TX/RX → DMA1 channel 4/5; USART2 RX/TX → DMA1 channel 6/7; I2C1 TX/RX → DMA1 channel 6/7. Because USART2 and I2C1 share channels 6 and 7, you cannot DMA both simultaneously on that device.

### 3.3.4 Modes and buffer ownership

- **Normal mode:** transfers a finite block, then stops and signals TC. Ownership of the buffer returns to the CPU at TC.
- **Circular mode:** wraps endlessly. **Half-transfer** signals that the first half is filled while DMA fills the second; **transfer-complete** signals the second half is filled while DMA wraps to the first. The CPU must finish a half before DMA returns to it. A callback does not freeze the buffer.
- **Memory-to-memory:** CPU-free block copies; on F1 it uses any channel with the MEM2MEM bit and cannot be circular.

**Listing 3.3 — Memory-to-memory DMA copy (names depend on the channel CubeMX generated)**

```c
static uint32_t src_block[256];
static uint32_t dst_block[256];
static volatile uint8_t g_copy_done;

void CopyWithDma(void)
{
    g_copy_done = 0U;
    /* The handle hdma_memtomem_dma1_channel1 is created when you add a
       MemToMem stream in CubeMX (DMA Settings -> MemToMem). */
    HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_channel1,
                             HAL_DMA_XFER_CPLT_CB_ID, CopyDoneCb);
    if (HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel1,
                         (uint32_t)src_block, (uint32_t)dst_block,
                         256U) != HAL_OK)         /* count = number of 32-bit items */
    {
        Error_Handler();
    }
}

void CopyDoneCb(DMA_HandleTypeDef *hdma) { (void)hdma; g_copy_done = 1U; }
```

### 3.3.5 DMA with the common peripherals

**ADC (peripheral → memory, circular).** The standard high-rate acquisition path; see Section 3.7.

**UART transmit (memory → peripheral, normal).** Non-blocking logging; the buffer must stay valid until the callback.

```c
static char tx_buf[64];                          /* static: must outlive the call */
int n = snprintf(tx_buf, sizeof(tx_buf), "T=%d.%02d\r\n", t_c, t_frac);
if (HAL_UART_Transmit_DMA(&huart2, (uint8_t *)tx_buf, (uint16_t)n) != HAL_OK) { /* BUSY: drop or queue */ }
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) { if (huart->Instance == USART2) { g_tx_busy = 0U; } }
```

**UART receive with idle-line detection** (variable-length packets with no per-byte interrupt):

```c
static uint8_t rx_dma_buf[128];
static volatile uint16_t g_rx_len;
static volatile uint8_t  g_rx_ready;

void Uart_StartRx(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_dma_buf, sizeof(rx_dma_buf));
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);   /* ignore half-transfer events */
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2) { g_rx_len = Size; g_rx_ready = 1U; }
    /* Main loop copies rx_dma_buf[0..Size-1], then re-arms with Uart_StartRx(). */
}
```

The API exists in recent HAL packages; **Verify** availability and re-arm rules in your family’s HAL release notes.

**SPI (full duplex).** Useful for displays and flash pages; chip select must stay low for the whole transfer and be released in the completion callback.

```c
HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_RESET);
HAL_SPI_TransmitReceive_DMA(&hspi1, tx_cmd, rx_data, len);
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) { HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_SET); g_spi_done = 1U; }
}
```

**I²C.** Register-block reads can use `HAL_I2C_Mem_Read_DMA()` with `HAL_I2C_MemRxCpltCallback()` so that a 14-byte IMU burst does not block. I²C DMA has family-specific errata and ordering rules; **Verify** in the errata sheet for your part and fall back to interrupt mode if needed.

### 3.3.6 Common DMA mistakes

- Wrong request/channel/stream, or `MX_DMA_Init()` ordered after the peripheral init.
- Mismatched data widths (32-bit DMA word into a 16-bit array corrupts neighbors).
- Memory increment disabled when it is needed (every item overwrites one location), or peripheral increment enabled when it should not be.
- Passing a **stack** array to an asynchronous transfer that outlives the function, or modifying a TX buffer while DMA reads it.
- DMA target memory not accessible to the DMA (for example certain RAM regions on H7).
- Cortex-M7 data-cache coherency ignored (Section 5.8).
- Doing slow work in a DMA callback, or ignoring TE/overrun events.
- Circular DMA overwriting a half before the CPU has finished with it.

## 3.4 Polling vs. Interrupts vs. DMA

![Figure 3.2 — Polling, interrupt-driven and circular DMA timelines](diagrams/fig3_polling_irq_dma.png)

*Figure 3.2 — Execution paradigms: (1) blocking polling; (2) interrupt per item; (3) timer + ADC + circular DMA with half-buffer ownership.*

**Table 3.3 — Trade-offs**

| Paradigm | CPU involvement | Latency / timing | Prerequisites | Best use | Primary failure |
| --- | --- | --- | --- | --- | --- |
| Polling | 100 % while waiting | Whenever you next poll | None | Init checks, 1 Hz sensors, labs | Lockup if a flag never sets; jitter |
| Interrupt | One ISR (tens to hundreds of cycles) per event | Microseconds, bounded by higher priorities | NVIC enable, handler, flag clearing | Buttons, UART commands, timer ticks, fault lines | ISR too fat; starvation; priority inversion |
| DMA | Setup plus one notification per block | Hardware-timed transfer; CPU notified at block edges | DMA request, buffer, width/increment settings | ADC streams, bulk UART/SPI, displays, audio | Wrong mapping; overwrite if the CPU is late; cache |

**Quantifying the CPU cost.** If one interrupt per item costs *t_isr* (entry, HAL dispatch, your code, exit; typically 1–3 µs for HAL callbacks at 72 MHz) and items arrive at rate *R*, CPU load ≈ *R × t_isr*. UART at 115 200 baud delivers 11 520 bytes/s, so with t_isr = 2 µs the load is 2.3 %, which is fine. An ADC at 100 kS/s with a per-sample interrupt costs 100 000 × 2 µs = **20 %**, and 500 kS/s would be impossible. With DMA at a 256-sample half buffer the same 100 kS/s costs about 2 × 391 interrupts per second, roughly 0.1 %.

**Table 3.4 — Starting point by requirement**

| Requirement | Likely approach |
| --- | --- |
| Read a sensor once every few seconds | Polling or simple interrupt |
| React to a button edge | EXTI interrupt with debounce |
| Periodic action | Timer interrupt, or timer-triggered peripheral event |
| Generate PWM | Timer hardware PWM (never bit-bang) |
| Stable-rate analog sampling | Timer-triggered ADC |
| Block of ADC samples, efficiently | Timer-triggered ADC + circular DMA |
| Continuous UART receive | RX interrupt with ring buffer, or DMA with idle-line detection |
| Tiny low-rate transfer | Polling may be simpler and sufficient |
| Command console | RX interrupt or DMA-to-idle plus a parser in `main` |
| 4 KB SPI flash page / display frame | DMA if the CPU has better work to do |

> [!NOTE]
> **Key concept.** Polling, interrupts and DMA are not rival technologies. They answer different questions: *how is the CPU told?* and *who moves the data?* A real pipeline combines them: a timer schedules the sample, the ADC converts, DMA stores, and an interrupt tells the application a block is ready.

## 3.5 When Should DMA Be Used?

**Use DMA when**

- samples must be acquired continuously or at a fixed rate (ADC streams, audio, vibration),
- data rate × per-item interrupt cost would consume a significant share of the CPU,
- you transfer large blocks (display frames, flash pages, log buffers),
- you need predictable data movement while the CPU does other work.

**DMA is unnecessary when**

- reads are occasional (1 Hz sensor), small and not time critical,
- the transfer is only a few bytes (setup overhead exceeds the copy),
- the project is simple and has no demanding timing requirement,
- you cannot afford the extra complexity of buffer lifetime and ownership.

**Performance considerations.** DMA and the CPU compete for the bus matrix and for SRAM banks; place high-rate buffers so that CPU code and data fetches do not collide on the same slave where the device allows. Choose block size from sample rate, processing time, RAM and latency: larger blocks reduce interrupt rate and per-block overhead but raise latency and RAM; smaller blocks reduce latency but increase callback frequency. DMA introduces its own complexity: buffer management, completion and error handling, and cache coherency on Cortex-M7 parts.
