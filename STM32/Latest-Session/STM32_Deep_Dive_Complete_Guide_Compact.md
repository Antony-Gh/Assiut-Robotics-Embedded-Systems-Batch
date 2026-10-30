# STM32 Deep-Dive Field Guide

From CubeMX configuration to deterministic data acquisition and FreeRTOS.

| Configure | Implement | Engineer |
|-----------|-----------|----------|
| Pins, clocks, peripherals | HAL, interrupts, DMA | Timing, drivers, RTOS |

A practical, family-aware reference for embedded-systems students and developers.

**Concepts · setup walkthroughs · code patterns · calculations · labs · debugging · review questions**

---

## How to use this guide

Treat each chapter as a repeatable engineering workflow:

1. Define the hardware problem.
2. Consult the **exact** MCU datasheet and reference manual.
3. Configure the peripheral.
4. Inspect generated code.
5. Test a minimal example.
6. Measure behavior.
7. Integrate only after the isolated path works.

> **Family-specific warning.** STM32 is a broad family. Pin mappings, clock trees, timer rules, ADC features, DMA routing, interrupt behavior, HAL APIs, and middleware vary by device and Cube package. Treat every code listing as a **pattern**, not a universal drop-in. Verify the exact MCU and board before wiring or relying on timing.

---

## Contents

1. [Learning objectives and the system view](#1-learning-objectives-and-the-system-view)
2. [STM32 tools, projects, and code generation](#2-stm32-tools-projects-and-code-generation)
3. [GPIO, alternate functions, and electrical behavior](#3-gpio-alternate-functions-and-electrical-behavior)
4. [Clock tree and clock calculations](#4-clock-tree-and-clock-calculations)
5. [Peripheral configuration and initialization](#5-peripheral-configuration-and-initialization)
6. [HAL vs LL vs direct-register programming](#6-hal-vs-ll-vs-direct-register-programming)
7. [Polling, interrupts, callbacks, and DMA](#7-polling-interrupts-callbacks-and-dma)
8. [Timers and deterministic timing](#8-timers-and-deterministic-timing)
9. [Timer-triggered ADC + DMA](#9-timer-triggered-adc--dma)
10. [UART: framing and robust streams](#10-uart-framing-and-robust-streams)
11. [SPI: modes, chip select, and timing](#11-spi-modes-chip-select-and-timing)
12. [I²C: addressing, pull-ups, and recovery](#12-i²c-addressing-pull-ups-and-recovery)
13. [Choosing a communication interface](#13-choosing-a-communication-interface)
14. [ADC, analog front ends, and accuracy](#14-adc-analog-front-ends-and-accuracy)
15. [Sampling, throughput, and data integrity](#15-sampling-throughput-and-data-integrity)
16. [Reusable sensor drivers](#16-reusable-sensor-drivers)
17. [Bare-metal superloops and state machines](#17-bare-metal-superloops-and-state-machines)
18. [FreeRTOS fundamentals and patterns](#18-freertos-fundamentals-and-patterns)
19. [Debugging playbook](#19-debugging-playbook)
20. [Seven practical labs](#20-seven-practical-labs)
21. [Engineering checklists and formulas](#21-engineering-checklists-and-formulas)
22. [Knowledge check with answers](#22-knowledge-check-with-answers)

---

# 1. Learning objectives and the system view

*Think in data flow, timing, and ownership — not just peripheral names.*

## The system as a chain

An STM32 combines a CPU, memory, clock/reset logic, buses, GPIO, timers, analog blocks, serial interfaces, interrupts, and often DMA. Firmware configures these blocks so physical signals become useful data and application decisions become outputs.

| Layer | Question | Example |
|-------|----------|---------|
| Physical / electrical | What signal exists? | A divider produces 1.8 V. |
| Peripheral | Which block measures or transfers it? | ADC converts voltage to a code. |
| Data movement | How does it reach RAM? | DMA writes conversions into a buffer. |
| Timing / notification | When should work happen? | Timer triggers; callback signals completion. |
| Application | What does the data mean? | Scale, filter, detect a threshold. |
| Output | How is the result delivered? | UART telemetry or SPI display. |

## Engineering questions

- **Correctness:** Does the value mean what the application thinks it means?
- **Timing:** When does an event happen, and what is its worst-case delay?
- **Throughput:** Can processing keep up with arriving data?
- **Resources:** CPU, RAM, flash, power, and bus bandwidth?
- **Robustness:** What happens on timeout, disconnect, malformed input, or overrun?
- **Maintainability:** Can the driver be tested without the entire application?

## Example: temperature telemetry

1. Sensor changes voltage or returns digital data.
2. ADC or a serial bus obtains raw data.
3. Timer or scheduler defines measurement timing.
4. Driver validates transport and converts raw data to documented units.
5. Application filters values and evaluates thresholds.
6. Telemetry sends a packet; diagnostics count errors and dropped samples.

---

# 2. STM32 tools, projects, and code generation

*Know which tool owns each part of the workflow.*

| Item | Responsibility | Misconception |
|------|----------------|---------------|
| STM32CubeMX | Pinout, clocks, peripheral settings, middleware, code generation. | It does not write the whole application. |
| STM32CubeIDE | Edit, build, link, flash, debug, inspect memory and registers. | Compilation does not prove runtime correctness. |
| `.ioc` | Stores CubeMX target and configuration choices. | It is not compiled firmware. |
| HAL / LL package | APIs and family-specific driver code. | Different family APIs may not match. |
| Startup / linker files | Reset entry, vector table, memory map. | Wrong memory map can break a valid build. |
| Debug probe | Flash and debug access. | Successful flash does not prove peripherals work. |

## Recommended workflow

1. Identify exact MCU part, package, board revision, and oscillator.
2. Create a project for that target.
3. Configure pins and resolve alternate-function conflicts.
4. Set the clock tree and verify limits.
5. Enable **one** peripheral at a time and configure mode, timing, IRQ, and DMA.
6. Generate code and inspect handles, init functions, and interrupt files.
7. Keep application logic in separate modules or in protected user-code blocks.
8. Build after each significant change; flash and test minimal behavior.
9. Commit `.ioc` and source files together; inspect diffs after regeneration.

## What generated code does

CubeMX turns configuration into initialization and project files. It may configure HAL, clocks, GPIO, DMA, NVIC, and peripherals. It does **not** design your state machine, define data ownership, frame UART messages, calibrate sensors, or decide recovery behavior.

- Regeneration can overwrite edits **outside** `USER CODE BEGIN` / `USER CODE END` sections.
- Separate `app.c`, `sensor.c`, and `sensor.h` modules are easier to test than large generated-file edits.
- After changing `.ioc`, inspect pins, clocks, DMA setup, IRQ enables, and initialization order.
- Treat compiler warnings as clues, not cosmetic output.

| Stage | What success proves | What it does not prove |
|-------|---------------------|------------------------|
| Compile | Source accepted by the compiler. | Correct wiring or timing. |
| Link | Symbols resolve and sections fit configured memory. | Linker matches the physical chip. |
| Flash | Image programmed. | Runtime behavior is correct. |
| Run | CPU executes instructions. | Application makes useful progress. |
| Debug | Selected state can be inspected. | Timing is unchanged under breakpoints. |

---

# 3. GPIO, alternate functions, and electrical behavior

*A pin is an electrical connection, not merely a software variable.*

| Mode | Behavior | Use |
|------|----------|-----|
| Input | Samples digital level. | Button or sensor output. |
| Push-pull | Actively drives high and low. | LED, chip select. |
| Open-drain | Pulls low or releases the line. | I²C and wired-AND lines. |
| Alternate function | Routes an internal peripheral signal. | UART, SPI, PWM, capture. |
| Analog | Disconnects much of the digital input path. | ADC or DAC where supported. |

## Alternate functions and floating inputs

A pin may support several alternate functions, but only the selected function is normally active. Package size affects which pins are bonded out. A digital input without a defined drive can float; pull-ups or pull-downs establish a default state. Internal pulls are convenient but weak and variable; external resistors may be required for bus timing or noise immunity.

EXTI (external interrupt) is a separate choice from GPIO mode. A pin can be an input **and** generate an interrupt on an edge; the NVIC must still enable the corresponding EXTI line.

## Voltage compatibility

- Do not assume every STM32 pin tolerates 5 V; check pin-specific datasheet limits and `FT` (five-volt tolerant) notes.
- Use level shifting or attenuation where required; check direction and speed behavior.
- Single-ended interfaces generally need a common ground reference.
- Do not drive motors, relays, or high-current loads directly from GPIO; use driver and protection circuitry.

```c
/* CubeMX normally generates GPIO initialization. Smoke test only. */
HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

while (1) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(250); /* coarse blocking wait, not a waveform generator */
}
```

This is a smoke test, not precision timing. LED polarity, wrong pin mapping, missing GPIO clock, or incorrect board assumptions can make correct code appear broken.

---

# 4. Clock tree and clock calculations

*Clock configuration affects CPU, timers, UART baud, ADC timing, and peripheral limits.*

| Term | Meaning |
|------|---------|
| HSI | Internal high-speed oscillator; accuracy varies by family and conditions. |
| HSE | External source, commonly a crystal/resonator or external clock. |
| LSI / LSE | Low-speed internal/external clocks, often used for RTC and independent watchdog. |
| PLL | Derives frequencies using allowed multipliers and dividers. |
| SYSCLK | Selected system clock. |
| HCLK | AHB clock for CPU/bus domains on many families. |
| PCLK1 / PCLK2 | APB peripheral bus clocks; mapping is device-specific. |
| Kernel clock | Some families let peripherals select independent clock sources. |

## Clock setup procedure

1. Confirm the actual board oscillator from the schematic.
2. Choose source and PLL settings within device limits.
3. Set AHB/APB prescalers and required voltage scaling / flash latency (wait states).
4. Check CPU, bus, timer, ADC, USB, and other peripheral limits.
5. Read `SystemClock_Config()` and its failure path (`Error_Handler()`).
6. Verify a supported clock output (MCO) or a measured peripheral signal when practical.

## Timer clock may differ from PCLK

On many STM32 families (classic F1/F4-style trees), if an APB prescaler is greater than one, timers on that APB may receive **twice** PCLK. This is common but not universal; newer families can use independent timer kernel clocks. Read the exact clock tree and reference manual.

## Worked timer calculation

For a common edge-aligned up-counter:

\[
f_{\text{update}} = \frac{f_{\text{timer}}}{(\text{PSC}+1)\times(\text{ARR}+1)}
\]

Example: timer input 72 MHz, PSC = 71, ARR = 999.

```text
f_timer   = 72_000_000 Hz
PSC       = 71
ARR       = 999
f_counter = 72_000_000 / (71 + 1) = 1_000_000 Hz
f_update  = 1_000_000 / (999 + 1) = 1_000 Hz
T_update  = 1 / 1_000 Hz = 1 ms
```

- Do not use SYSCLK in a timer formula without checking the timer's actual clock.
- Do not forget `+1` in PSC and ARR.
- Do not assume oscillator frequency from a tutorial.
- After clock changes, recheck UART baud, ADC timing, and peripheral limits.
- Respect oscillator startup, flash latency, and voltage scaling.

---

# 5. Peripheral configuration and initialization

*Settings are contracts among MCU, driver, and external device.*

| Area | Verify | Why |
|------|--------|-----|
| Pinout | Physical pin, AF, pulls, speed, output type. | Wrong pin mimics software failure. |
| Clock | Source and frequency. | Timing and baud can be wrong. |
| Mode | Master/slave, direction, edge, frame. | Mismatch may fail silently. |
| Timing | Baud, prescaler, sample time, timeout. | Mode alone is insufficient. |
| NVIC | IRQ enable, priority, handler route. | Event may not reach the callback. |
| DMA | Request, direction, width, increment, mode, alignment. | Bad settings corrupt or stall transfers. |
| Electrical | Pull-ups, CS/reset, voltage. | Firmware cannot fix wiring. |
| Errors | Timeout and recovery policy. | A fault must not freeze the system. |

## Initialization order

Generated order varies. A common pattern initializes HAL and clocks, then GPIO, DMA, and peripherals. External sensors may require power-up delay, reset release, identity checks, and configuration writes. Start continuous acquisition only after buffer, callback path, and application state are ready.

- Do not start DMA before its request/channel and peripheral are initialized.
- Check every sensor transaction; do not use stale or uninitialized data after failure.
- Make initialization failure visible via logging, error indication, retry, or a safe state.
- Use bounded timeouts so a disconnected device cannot freeze the whole application.

```c
uint8_t id = 0;

if (HAL_I2C_Mem_Read(&hi2c1, sensor_addr, WHO_AM_I_REG,
                     I2C_MEMADD_SIZE_8BIT, &id, 1, 100) != HAL_OK) {
    sensor_online = false; /* id is not trusted after failure */
}
```

Many HAL calls return `HAL_OK`, `HAL_ERROR`, `HAL_BUSY`, or `HAL_TIMEOUT`. Check the result and, when useful, the peripheral error code (`HAL_*_GetError()`).

---

# 6. HAL vs LL vs direct-register programming

*Choose abstraction based on requirements, not ideology.*

| Level | Characteristics | Good fit |
|-------|-----------------|----------|
| HAL | Higher-level APIs and common status handling. | Most application firmware and learning. |
| LL | More direct peripheral access, still generated by Cube. | When HAL API or overhead is unsuitable. |
| Registers | Direct hardware register access. | Special control needs or low-level study. |

## Trade-offs

HAL calls may add function and state-check overhead, but hardware still performs conversions, timer counting, and serial shifting. Often peripheral timing dominates. Measure before replacing maintainable code.

- Reserved bits and write-one-to-clear flags need correct handling.
- Clock gating and reset state must be correct.
- Registers differ across STM32 families.
- Read-modify-write may lose updates if another context touches the register.
- Bypassing HAL may desynchronize its internal state or pending flags.

Recommended path:

1. Start with CubeMX + HAL and make a minimal example work.
2. Inspect generated code and HAL source to understand hardware behavior.
3. Read the reference manual and measure performance.
4. Optimize only after identifying a measured bottleneck.
5. Isolate low-level code behind a small interface and test on the exact target.

---

# 7. Polling, interrupts, callbacks, and DMA

*These are different ways to coordinate CPU and hardware.*

## Polling

Polling repeatedly checks a flag or waits for a blocking call. It is simple for startup, low-rate sensors, and short operations. The CPU may waste time waiting, and response depends on check frequency.

## Interrupts

An interrupt lets hardware notify the CPU. HAL often wraps the low-level handler (`USARTx_IRQHandler`) and calls a callback (`HAL_UART_RxCpltCallback`). Callback code triggered by an interrupt runs in **interrupt context**; its execution time contributes to latency and jitter.

## DMA

DMA transfers data between peripheral and memory without the CPU copying every item. It helps sustained ADC streams, serial reception, and long transfers. The CPU still configures it, handles completion and errors, and processes data.

| Method | CPU role | Trade-off |
|--------|----------|-----------|
| Polling | Repeatedly checks or waits. | Simple but can block. |
| Interrupt | Services event / callback. | Responsive, but ISR duration and priority matter. |
| DMA | Sets up transfer; handles boundaries and errors. | Efficient streams, but ownership must be designed. |

## ISR rules and `volatile`

- Acknowledge or capture the event and record **minimal** state.
- Notify foreground / task and return quickly.
- Avoid `printf`, long delays, blocking HAL calls, allocation, and complex filtering in an ISR.
- `volatile` prevents some compiler optimizations but does **not** make compound operations atomic, protect structures, or prevent DMA overwrites.

```c
static volatile uint8_t sample_ready = 0;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        sample_ready = 1;
    }
}

/* Foreground code must still define safe buffer ownership. */
if (sample_ready) {
    sample_ready = 0;
    process_samples();
}
```

| Requirement | Starting point |
|-------------|----------------|
| Button / slow flag | Polling or EXTI interrupt |
| Short occasional sensor read | Blocking HAL with timeout |
| Variable-length UART | Interrupt / DMA plus parser |
| High-rate ADC | Timer trigger + ADC + DMA |
| Large SPI transfer | DMA if justified |
| Precise waveform | Timer / PWM hardware |

---

# 8. Timers and deterministic timing

*Timers are hardware counters, not merely software delays.*

| Element | Purpose |
|---------|---------|
| CNT | Current counter value. |
| PSC | Divides timer input clock. |
| ARR | Terminal count / update period in common up-counting modes. |
| CCR | Compare value for output compare / PWM. |
| Input capture | Latches the counter at an edge to measure pulse timing. |
| Update event | Overflow / underflow or software-generated event, depending on mode. |
| PWM | Hardware output with period and duty. |
| One-pulse | Bounded pulse / event sequence. |

## Frequency and PWM

For common edge-aligned up-counting:

\[
f_{\text{update}} = \frac{f_{\text{timer}}}{(\text{PSC}+1)(\text{ARR}+1)}
\]

For common edge-aligned PWM, duty is approximately \(\text{CCR}/(\text{ARR}+1)\), subject to mode, polarity, preload, and edge conventions.

With a 1 MHz counter and ARR = 999, update period is 1 ms; CCR = 250 gives about 25% duty.

```c
if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) {
    Error_Handler();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        tick_count++; /* keep this short */
    }
}
```

## Setup and pitfalls

1. Choose the timer function: interrupt, PWM, capture, output compare, encoder, or trigger.
2. Verify the actual timer clock.
3. Calculate PSC / ARR and check counter width (16-bit vs 32-bit).
4. Configure pin alternate function if using a channel.
5. Enable NVIC only if software needs the interrupt; start with the matching HAL API (`HAL_TIM_Base_Start`, `_IT`, `HAL_TIM_PWM_Start`, …).

Common mistakes:

- Wrong timer clock or APB multiplier.
- Confusing update rate with PWM frequency or callback rate.
- Forgetting the start call or IRQ route.
- Long callback causes jitter.
- Counter width too small for the required interval.

`HAL_Delay` is a coarse blocking wait (typically based on SysTick). Use timer hardware for pulses, capture, PWM, and accurate periodic events.

---

# 9. Timer-triggered ADC + DMA

*The central acquisition example: timer schedules, ADC converts, DMA stores, software processes.*

| Block | Responsibility |
|-------|----------------|
| Timer | Defines repeatable sample timing. |
| ADC | Samples voltage and produces a digital code. |
| DMA | Moves conversion data to RAM. |
| DMA callback | Signals completed region or error. |
| Application | Scales, filters, validates, logs, reacts. |

## Configuration sequence

1. Choose ADC pin and analog mode.
2. Select channel, resolution, sampling time, and conversion mode supported by the exact ADC.
3. Select a **supported** timer trigger and edge.
4. Calculate timer rate from the actual clock.
5. Configure DMA request, direction, widths, increment, circular/normal mode, alignment.
6. Enable the needed ADC / DMA IRQ path.
7. Allocate a correctly typed / aligned buffer with correct length and lifetime.
8. Start ADC / DMA and the trigger timer in the **documented family-specific order**.
9. Validate with safe known DC and waveform inputs.

## Buffer deadline

At 10,000 samples/s, 200 samples span 20 ms. Half / full callbacks divide this into 100-sample regions of 10 ms each. Processing must finish **before** DMA returns to overwrite that region.

```c
#define ADC_BUFFER_SAMPLES 200

static uint16_t adc_buffer[ADC_BUFFER_SAMPLES];
static volatile uint8_t adc_half_ready;
static volatile uint8_t adc_full_ready;

if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer,
                      ADC_BUFFER_SAMPLES) != HAL_OK) {
    Error_Handler();
}

/* Start the timer trigger after configuring acquisition. */
if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
    Error_Handler();
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        adc_half_ready = 1;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        adc_full_ready = 1;
    }
}
```

In the main loop (or a task), process `adc_buffer[0 .. N/2)` on half-complete and `adc_buffer[N/2 .. N)` on full-complete. Do the heavy work **outside** the ISR.

## Code to voltage

A common ideal estimate is:

\[
V_{\text{in}} \approx \text{code} \times \frac{V_{\text{ref}}}{2^{N}-1}
\]

For 12 bits, code 2048 and \(V_{\text{ref}} = 3.3\,\text{V}\) gives about 1.65 V. Real results depend on transfer-function convention, reference error, calibration, input impedance, sampling time, noise, and layout.

## Race conditions and validation

A ready flag does **not** freeze DMA memory. Circular DMA can overwrite a region while software reads it. Use measured deadlines, double buffering, a queue of completed-block descriptors, or a copy into consumer-owned memory. On MCUs with data cache (many Cortex-M7 parts such as STM32H7), follow device-specific cache coherency guidance.

- Feed safe known DC voltages and compare expected code ranges.
- Measure the actual pin voltage and trigger / sample timing.
- Count callbacks, errors, overruns, and samples.
- Raise sample rate gradually and measure worst-case processing time.

---

# 10. UART: framing and robust streams

*UART is asynchronous: both endpoints must agree on baud and frame format.*

## Frame and throughput

A common format is 115200 baud, 8-N-1: 8 data bits, no parity, one stop bit. With one start and one stop bit, each byte uses 10 bit-times, so ideal payload throughput is about **baud / 10** bytes/s before protocol framing.

- Connect TX(A) to RX(B), RX(A) to TX(B), and common ground.
- Confirm logic voltage: MCU UART is not automatically RS-232 voltage.
- Identify the USB-UART bridge or virtual COM path and connector pinout.
- Check flow control and both ends' baud / framing.

```c
const uint8_t msg[] = "STM32 ready\r\n";

HAL_UART_Transmit(&huart2, (uint8_t *)msg, sizeof(msg) - 1, 100);

/* Non-blocking alternative; buffer must remain valid until completion. */
HAL_UART_Transmit_IT(&huart2, (uint8_t *)msg, sizeof(msg) - 1);
```

## Receive is a stream

UART does not inherently mark command boundaries. A callback can contain part of a command, one command, or several. Define framing with a delimiter, fixed length, length field, idle-line detection, or a robust packet protocol.

```c
static uint8_t rx_byte;
static char line[64];
static size_t line_len;
static volatile uint8_t command_ready;
static uint32_t rx_overflow_count;

void start_rx(void)
{
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) {
        return;
    }

    char c = (char)rx_byte;

    if (c == '\n') {
        line[line_len] = '\0';
        command_ready = 1;
        line_len = 0;
    } else if (c != '\r') {
        if (line_len < sizeof(line) - 1) {
            line[line_len++] = c;
        } else {
            line_len = 0;
            rx_overflow_count++;
        }
    }

    HAL_UART_Receive_IT(&huart2, &rx_byte, 1); /* re-arm */
}
```

For higher rates, prefer DMA with idle-line detection (`HAL_UARTEx_ReceiveToIdle_DMA` on families that support it) plus a parser in the foreground.

## Debugging

- At 115200 baud, bit time is about 8.68 µs; measure it if output is garbled.
- If only the first byte works, verify re-arming or DMA / receive-to-idle setup.
- If bytes disappear, inspect framing, buffer overflow, ISR load, DMA wraparound, and cache coherency on cached MCUs.

---

# 11. SPI: modes, chip select, and timing

*SPI is synchronous and commonly used for displays, flash, ADCs, and sensors.*

| Signal | Role |
|--------|------|
| SCK | Clock, usually generated by the master. |
| MOSI | Master Out, Slave In. |
| MISO | Master In, Slave Out. |
| CS / NSS | Selects the intended slave, often active-low. |
| GND | Common signal reference. |

## CPOL and CPHA

CPOL sets clock idle level; CPHA sets sampling edge relative to clock transitions. SPI modes 0–3 combine these choices. Use the device timing diagram; a mismatch can produce plausible but incorrect bytes.

| Mode | CPOL | CPHA | Description |
|------|------|------|-------------|
| 0 | 0 | 0 | Idle low, sample first edge. |
| 1 | 0 | 1 | Idle low, sample second edge. |
| 2 | 1 | 0 | Idle high, sample first edge. |
| 3 | 1 | 1 | Idle high, sample second edge. |

## Full duplex and dummy bytes

SPI shifts both directions at once. To receive, the master usually generates clocks by transmitting dummy bytes. Command / address phase, dummy clocks, CS duration, and gaps are device-specific.

GPIO-controlled CS is often clearer than hardware NSS for multi-byte transactions, because you can hold CS low across the whole command.

```c
uint8_t tx[2] = { READ_COMMAND, 0x00 };
uint8_t rx[2] = { 0 };

HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 100);
HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);

if (st != HAL_OK) {
    /* Record error and recover. */
}
```

- Start with a conservative SCK and increase only within sensor limits.
- Capture CS, SCK, MOSI, and MISO together on a logic analyzer.
- Verify mode, bit order, word size, command order, CS polarity, and MISO behavior.
- Use DMA for long transfers only when useful and buffer lifetimes are correct.

---

# 12. I²C: addressing, pull-ups, and recovery

*Two shared signal wires reduce pin count but require correct electrical behavior.*

## Open-drain and pull-ups

SCL / SDA are typically open-drain: devices pull low or release the line. Pull-ups establish the high level and permit sharing. Resistance trades rise time against sink current; capacitance, bus speed, supply voltage, and device limits determine suitable values. Common starting points on short 3.3 V buses are 2.2 kΩ to 10 kΩ; **measure** rise time rather than copying a value blindly.

## Address conventions

Datasheets usually show a **7-bit** address; many STM32 HAL I²C APIs expect it **shifted left one bit**. Confirm the API and avoid shifting twice.

```c
#define SENSOR_ADDR_HAL  (SENSOR_ADDR_7BIT << 1)

uint8_t value = 0;
HAL_StatusTypeDef st = HAL_I2C_Mem_Read(
    &hi2c1, SENSOR_ADDR_HAL, REG_STATUS,
    I2C_MEMADD_SIZE_8BIT, &value, 1, 100);

if (st != HAL_OK) {
    uint32_t err = HAL_I2C_GetError(&hi2c1);
    /* Record err; do not trust value after failure. */
    (void)err;
}
```

| Symptom | Likely checks |
|---------|---------------|
| Address NACK | Wrong address / shift, unpowered device, reset still active. |
| SDA stuck low | Interrupted transaction, short, pull-up or power issue. |
| Intermittent errors | Slow rise time, bus speed, long wires, noise, grounding. |
| Wrong data | Register width, endianness, repeated-start, device variant. |
| Timeout / busy | Stuck bus, missing recovery, timing or driver-state issue. |

## Recovery

A system may stop new transactions, inspect SDA / SCL, and — if hardware and device permit — temporarily drive SCL as GPIO for a limited recovery sequence and STOP-like condition, then reinitialize I²C. Do not blindly apply generic recovery: follow the MCU peripheral and sensor documentation.

---

# 13. Choosing a communication interface

*Select based on device support, throughput, wiring, topology, cable length, and noise.*

| Interface | Typical signals | Strength | Trade-off |
|-----------|-----------------|----------|-----------|
| UART | TX / RX / GND | Simple point-to-point; excellent logs and commands. | No inherent addressing or framing. |
| SPI | SCK / MOSI / MISO / CS | Fast, full duplex, low protocol overhead. | More wires; CS and timing are device-specific. |
| I²C | SCL / SDA / GND | Many addressed devices on two signal wires. | Pull-ups, capacitance, recovery. |
| USB / CAN / RS-485 | Transceiver-specific | PC connectivity, robust multi-node or longer links. | Extra hardware and protocol stack. |

## Decision sequence

1. Read the device datasheet for supported interfaces.
2. Estimate data rate, transaction size, and latency.
3. Count pins and decide whether devices share a bus.
4. Check cable length, noise, grounding, and robustness.
5. Compare firmware complexity and available debug tools.
6. Start conservatively, verify, then increase performance if needed.

UART 8-N-1 payload is roughly baud / 10 bytes/s before framing. SPI raw transfer is near clock bit rate but loses time to commands, gaps, and CS. I²C adds address / ACK bits and start / stop conditions. **Nominal clock is not application payload throughput.**

---

# 14. ADC, analog front ends, and accuracy

*An ADC produces a code; it does not automatically measure a physical unit.*

| Concept | Meaning |
|---------|---------|
| Resolution | Number of codes; 12-bit gives 4096 values (0–4095). |
| Quantization step | Often quoted as \(V_{\text{ref}}/2^{N}\) (e.g. 3.3 / 4096 ≈ 0.806 mV) **or** \(V_{\text{ref}}/(2^{N}-1)\). Follow the device transfer function. |
| Accuracy | Closeness to true input, including reference, offset, gain, nonlinearity, circuit errors. |
| Repeatability | How tightly repeated values cluster; repeatable can still be biased. |
| Effective bits | Useful resolution after noise / distortion in a defined setup. |

## Code to voltage

A common ideal estimate is \(V_{\text{in}} \approx \text{code} \times V_{\text{ref}} / (2^{N}-1)\). Some step-size calculations use \(2^{N}\) in the denominator. Follow the device transfer-function definition and calibration guidance for precision work.

## Sample time and source impedance

An ADC sample-and-hold capacitor must charge during the sample phase. A high-impedance source may not settle in a short sampling window, causing bias or dependence on the previously sampled channel. Increase sample time, buffer the source, or change the analog front end.

- Reference-voltage error directly affects calculated voltage.
- Follow enable / settling / calibration steps for internal reference or temperature channels.
- Averaging reduces uncorrelated noise, not fixed offset, gain error, nonlinearity, or aliasing.
- Consider grounding, analog return paths, layout, and switching noise.

## Voltage divider

\[
V_{\text{node}} = V_{\text{supply}} \times \frac{R_{\text{bottom}}}{R_{\text{top}} + R_{\text{bottom}}}
\]

Divider loading, resistor tolerance, temperature coefficient, and source impedance affect accuracy. A thermistor divider is nonlinear and needs a suitable equation or lookup table.

## Aliasing and safe test plan

Signal content above half the sample rate can fold into lower frequencies (Nyquist). Use analog low-pass filtering where required; digital filtering cannot reliably remove already-aliased components.

1. Check pin, channel, analog mode, range, and ground.
2. Measure voltage **at the MCU pin**.
3. Read a stable input at conservative sample time.
4. Compare several safe voltages and inspect noise, offset, gain, and saturation.
5. Only then add DMA, filtering, scaling, and telemetry.

---

# 15. Sampling, throughput, and data integrity

*Every stream has an arrival rate, storage budget, and deadline.*

## Rates and buffer duration

Sample period \(T_{s} = 1/F_{s}\). At 10 kS/s, one sample arrives every 100 µs. Buffer duration = sample count / sample rate; 1024 samples at 8 kS/s hold 128 ms.

Raw bandwidth = samples/s × bytes/sample × channels. Four channels at 20 kS/s and 2 bytes/sample require 160,000 bytes/s **before** metadata, framing, and copies.

| Strategy | How it works | Main risk |
|----------|--------------|-----------|
| Single buffer | One region produced and consumed. | Race with a continuous producer. |
| Double / ping-pong | Producer fills one region while consumer handles the other. | Consumer must finish before wrap. |
| Ring buffer | Producer and consumer advance indices. | Define overflow and atomic updates. |
| Queue of blocks | Pass completed block descriptors. | Size the queue and define drop policy. |
| Copy on completion | Copy into consumer-owned memory. | More CPU / bandwidth; simpler ownership. |

## Overrun policy

- Drop newest or oldest **deliberately**, depending on whether history or latest state matters.
- Use backpressure only if the producer can safely pause.
- Stop and fail safe if dropped data invalidates the measurement.
- Count and expose overruns; silent loss is a defect.

On MCUs with data cache, CPU and DMA may observe different memory versions unless memory attributes / cache maintenance are correct. Use family-specific non-cacheable regions or documented clean / invalidate operations, alignment, and barriers.

---

# 16. Reusable sensor drivers

*A driver hides device details without hiding failures.*

| Layer | Owns | Avoid |
|-------|------|-------|
| Bus layer | I²C / SPI transfer primitives, timeouts, bus state. | Application policy. |
| Sensor driver | Register map, init, decode, units, device errors. | Global app variables. |
| Acquisition service | Scheduling, publication, buffering, health state. | Duplicated register decoding. |
| Application | Filtering, thresholds, UI, telemetry decisions. | Direct register knowledge. |

## API design rules

- Pass bus handles / interfaces explicitly.
- Document units and scaling.
- Return status; publish output only on success.
- Document integer width, signedness, and endianness.
- Separate initialization from reads when needed.
- Use bounded timeouts and identity checks.
- Avoid dynamic allocation unless justified; keep the public header small.

```c
/* Public interface: adapt includes and types to the MCU. */
typedef enum {
    TEMP_OK = 0,
    TEMP_ERR_ARGUMENT,
    TEMP_ERR_BUS,
    TEMP_ERR_ID,
    TEMP_ERR_CONFIG
} TempStatus;

typedef struct {
    I2C_HandleTypeDef *i2c;
    uint16_t address_hal; /* Document shifted / unshifted convention. */
} TempSensor;

TempStatus TempSensor_Init(TempSensor *dev);
TempStatus TempSensor_ReadCelsius(TempSensor *dev, float *temperature_c);

/* Implementation pattern; registers and scaling are placeholders. */
TempStatus TempSensor_ReadCelsius(TempSensor *dev, float *temperature_c)
{
    uint8_t b[2];

    if (dev == NULL || dev->i2c == NULL || temperature_c == NULL) {
        return TEMP_ERR_ARGUMENT;
    }

    if (HAL_I2C_Mem_Read(dev->i2c, dev->address_hal, TEMP_REG_MSB,
                         I2C_MEMADD_SIZE_8BIT, b, 2, 100) != HAL_OK) {
        return TEMP_ERR_BUS;
    }

    int16_t raw = (int16_t)(((uint16_t)b[0] << 8) | b[1]);
    *temperature_c = (float)raw * TEMP_LSB_C; /* datasheet scale */
    return TEMP_OK;
}
```

## Driver tests

- Test null arguments and invalid states.
- Mock bus NACK, timeout, and malformed data.
- Test byte order, signed values, and unit conversion.
- Test repeated reads, init failure, disconnect, and recovery.
- Keep driver tests separate from application policy.

---

# 17. Bare-metal superloops and state machines

*A well-designed superloop can be simpler and more reliable than an unnecessary RTOS.*

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    App_Init();

    while (1) {
        App_ProcessCommands();
        App_ProcessSensorResults();
        App_UpdateStateMachine();
        App_ServiceTelemetry();
    }
}
```

## What makes it reliable?

- Functions return quickly and do bounded work.
- Long operations are state-machine steps, not blocking delays.
- Interrupts / DMA capture time-critical events; the loop handles slower work.
- Each function has a known worst-case execution time and service rate.
- Shared state has clear ownership and synchronization.

A non-blocking sensor transaction can use `IDLE`, `START`, `WAIT_READY`, `READ_RESULT`, and `ERROR_RECOVERY` states. Each loop advances when a condition or deadline is met.

A superloop becomes awkward when independent deadlines conflict, one blocking operation delays unrelated work, or many producer / consumer relationships need queues and priorities.

---

# 18. FreeRTOS fundamentals and patterns

*An RTOS organizes concurrent work; it does not automatically make code real-time.*

| Concept | Meaning | Example |
|---------|---------|---------|
| Task | Execution context with stack and priority. | Sensor or telemetry task. |
| Scheduler | Chooses the ready task. | Higher priority may preempt. |
| Tick | Timing-service time base. | Delay / timeout. |
| Queue | Transfers messages / data. | Samples to telemetry. |
| Semaphore | Signals an event or resource. | DMA completion. |
| Mutex | Protects a shared resource between tasks. | Serialize bus access. |
| Notification / event group | Compact event signaling. | Sensor ready / link state. |
| Stack | Task-private call / local memory. | Too small may fault or corrupt. |

## Task design

Give each task one responsibility, a justified priority, a stack budget, blocking behavior, and a communication contract. Too many tasks increase stacks, context switches, and synchronization paths.

```c
typedef struct {
    uint32_t timestamp_ms;
    int16_t temperature_centi_c;
    uint16_t flags;
} SensorSample;

void SensorTask(void *argument)
{
    SensorSample sample;

    (void)argument;
    for (;;) {
        if (Sensor_ReadSample(&sample) == SENSOR_OK) {
            /* If the queue is full, this wait times out and the sample is dropped. */
            (void)xQueueSend(sensor_queue, &sample, pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void TelemetryTask(void *argument)
{
    SensorSample sample;

    (void)argument;
    for (;;) {
        if (xQueueReceive(sensor_queue, &sample, portMAX_DELAY) == pdPASS) {
            Telemetry_SendSample(&sample);
        }
    }
}
```

## Synchronization and memory

- **Queue:** receiver needs the data. **Semaphore / notification:** receiver needs an event. **Mutex:** tasks need exclusive resource access.
- Use documented `FromISR` APIs from interrupt context; never call mutex APIs from an ISR.
- A high-priority task that never blocks can starve others.
- Mutex priority inheritance can help priority inversion, but design still matters.
- Check task / queue creation results, monitor stack high-water marks, and enable overflow checks during development.
- Avoid large automatic arrays in tasks; allocate buffers deliberately.
- Do not call blocking HAL functions with long timeouts from high-priority tasks unless that is an explicit design choice.

| RTOS is justified by | Weak reason |
|----------------------|-------------|
| Independent tasks with different deadlines. | “Every embedded project needs an RTOS.” |
| Queues and blocking waits. | Hide blocking code by adding tasks. |
| Priority requirements / middleware architecture. | Create a task for every function. |

---

# 19. Debugging playbook

*Start with observable facts; change one variable at a time.*

1. **Power / wiring:** supply, ground, reset, pinout, voltage, pulls, connectors.
2. **Clock / pins:** oscillator, bus clocks, alternate function, GPIO mode.
3. **Peripheral settings:** mode, prescaler, width, edge, trigger, address, sample time.
4. **Data movement:** DMA route, NVIC, handler, callback, buffer size / alignment.
5. **Application:** framing, units, state, ownership, races, timeouts.

| Symptom | First checks |
|---------|--------------|
| Never reaches `main` | Reset / startup, debugger halt, watchdog, fault handler, linker target. |
| LED not blinking | Pin / polarity, GPIO clock / mode, board schematic, SysTick. |
| UART garbage | Baud / framing, clock, TX / RX, ground, voltage. |
| I²C absent | Power, pull-ups, address, pins, speed, waveform. |
| SPI all 0 / 1 | CS, mode, SCK, MOSI / MISO, dummy clocks, power / reset. |
| ADC fixed | Pin voltage, channel, analog mode, reference, start, alignment. |
| DMA callback absent | Request, NVIC, handler, start API, transfer length, error flags. |
| Timer wrong rate | Actual timer clock, APB rule, PSC / ARR `+1`, measurement. |
| Intermittent freeze | Infinite wait, long ISR, stack, deadlock, buffer overrun, watchdog. |
| Works only in debugger | Timing race, uninitialized memory, watchdog, debug-dependent behavior. |

## Tools and fault reporting

- **Debugger:** breakpoints, call stack, memory / register views, fault registers (`CFSR`, `HFSR`, stacked PC on Cortex-M).
- **Logic analyzer:** decode UART / SPI / I²C and inspect timing.
- **Oscilloscope:** levels, rise time, analog noise, clock, PWM, sample timing.
- **Multimeter:** supplies, ground continuity, static levels, pull-ups when appropriate.
- **Compiler warnings / static analysis:** types, uninitialized data, overflow, suspicious control flow.

During development, a breakpoint in `Error_Handler` and `HardFault_Handler` is useful. Field firmware may need fault records, reset reason, watchdog strategy, safe outputs, and controlled recovery. Avoid silently swallowing errors.

---

# 20. Seven practical labs

*Each lab has a target behavior and an observable acceptance test.*

## Lab 1 — GPIO heartbeat

1. Create a project for the exact board; identify the LED pin from the schematic.
2. Configure push-pull output and toggle the LED.
3. If it fails, verify pin / polarity, clock, init, and main-loop progress.

**Acceptance:** LED toggles after reset without needing the debugger.

## Lab 2 — UART console

1. Configure pins, baud, 8-N-1, and terminal connection.
2. Transmit a startup banner and verify externally.
3. Receive a command with interrupt / DMA; add framing, bounds, timeout, overflow counter.

**Acceptance:** Multiple valid and invalid commands are handled safely.

## Lab 3 — Timer timing

1. Calculate the actual timer input clock.
2. Choose PSC / ARR for the target period.
3. Toggle a diagnostic GPIO or use output compare; measure the period.

**Acceptance:** Measured rate matches calculation within expected tolerance.

## Lab 4 — ADC polling

1. Configure analog pin / channel / resolution / sample time.
2. Read raw code and scale to voltage.
3. Test several safe known input levels and record error / noise.

**Acceptance:** Values respond monotonically and do not saturate unexpectedly.

## Lab 5 — ADC + DMA

1. Configure timer trigger, ADC trigger, DMA width / increment / circular mode.
2. Start transfer; use half / full callbacks to notify processing.
3. Calculate buffer duration and verify the consumer finishes before overwrite.

**Acceptance:** Stable sample rate, correct data, no unexplained overruns.

## Lab 6 — Sensor driver

1. Read address, ID, init sequence, data format, conversion equation.
2. Implement Init / Read API with explicit dependencies and timeouts.
3. Test valid ID, normal reading, and disconnected sensor.

**Acceptance:** App uses the public API rather than raw registers.

## Lab 7 — FreeRTOS producer / consumer

1. Create sensor and telemetry tasks with justified priorities / stacks.
2. Pass samples via a queue with a full-queue policy.
3. Check RTOS API results and monitor stack usage.

**Acceptance:** Acquisition and telemetry run concurrently with observable error behavior.

## Integrated capstone

Connect timer-triggered sampling, ADC / DMA buffer, unit conversion, queue / ring buffer, and UART telemetry. Include timestamps / sequence numbers, sample count, overrun count, and clear fault state. Demonstrate rate, units, ownership, and fault behavior — not merely that numbers appear.

---

# 21. Engineering checklists and formulas

*Use these before demos, code reviews, and hardware integration.*

## Before enabling a peripheral

- Exact MCU / board and package confirmed.
- Pin mapping and alternate function checked against the datasheet.
- Clock source and peripheral clock verified.
- Mode / timing matches the datasheet.
- Electrical levels, grounds, pulls, reset pins verified.
- IRQ / DMA setup and callback route reviewed.
- Timeouts, error behavior, and test points defined.

## Before using DMA

- Correct request / channel and direction.
- Peripheral / memory widths and increments match the representation.
- Circular / normal mode matches the intended flow.
- Buffer size, alignment, and lifetime are correct.
- Consumer deadline is shorter than the overwrite interval.
- Cache coherency is handled on cached MCUs.
- Errors and overruns are visible / counted.

## Before shipping a driver

- Public API and units documented.
- Identity / config checked where appropriate.
- Every bus operation has a bounded timeout.
- Signedness, endianness, scaling, and invalid values handled.
- Failure never returns stale data as valid.
- No hidden global application dependency.
- Tests cover success, timeout, bad ID, and malformed data.

| Quantity | Formula / rule | Caution |
|----------|----------------|---------|
| Timer update | \(f_{\text{timer}}/((\text{PSC}+1)(\text{ARR}+1))\) | Common up-counter; verify family. |
| Timer period | \(T = 1/f\) | Use consistent units. |
| PWM duty | about \(\text{CCR}/(\text{ARR}+1)\) | Mode / polarity affect edge cases. |
| Sample period | \(T_{s} = 1/F_{s}\) | 10 kS/s = 100 µs. |
| Buffer duration | \(N_{\text{samples}}/F_{s}\) | Half-buffer uses half the count. |
| Raw bandwidth | \(F_{s} \times\) bytes/sample \(\times\) channels | Add metadata / protocol separately. |
| ADC code | \(V_{\text{in}} \times (2^{N}-1)/V_{\text{ref}}\) | Approximation; calibrate as needed. |
| UART 8-N-1 | about baud / 10 bytes/s | Ignores higher-level framing. |
| Bus throughput | Count bits, gaps, commands, ACKs | Clock is not payload rate. |

## Code hygiene

- Name variables with units: `sample_rate_hz`, `timeout_ms`, `temperature_c`.
- Use fixed-width types where width matters (`uint16_t`, `int32_t`).
- Replace magic numbers with named constants and document derived values.
- Check overflow in sample-count / rate / unit multiplications.
- Separate hardware init, driver, acquisition service, and application.
- Expose diagnostics without flooding time-critical paths.

---

# 22. Knowledge check with answers

*Use this as a revision sheet or oral assessment.*

**1. What does `.ioc` store?**  
CubeMX target, pins, clocks, peripheral settings, and generation options.

**2. Why can regeneration overwrite code?**  
Edits outside protected `USER CODE` sections may be replaced; config changes alter initialization.

**3. CubeMX vs CubeIDE?**  
CubeMX configures and generates; CubeIDE edits, builds, flashes, and debugs. (CubeMX also exists as a standalone tool.)

**4. When is polling appropriate?**  
Simple low-rate or short operations where blocking is acceptable.

**5. Interrupt vs DMA?**  
Interrupt notifies the CPU; DMA moves data. They often work together.

**6. Why is `volatile` insufficient for shared buffers?**  
It provides neither atomicity, ownership, locking, nor protection from DMA overwrite.

**7. Why can timer clock differ from SYSCLK?**  
Bus prescalers and peripheral kernel-clock selection affect actual timer input.

**8. Why PSC+1 and ARR+1?**  
Common timer registers encode divisor / period minus one.

**9. What do CPOL and CPHA control?**  
SPI clock idle polarity and sampling-edge relationship.

**10. Why I²C pull-ups?**  
Open-drain lines release high rather than actively driving high.

**11. Why UART garbage?**  
Baud / framing, clock, wiring, voltage, or terminal mismatch.

**12. Resolution vs accuracy?**  
Resolution is code count; accuracy is closeness to true input.

**13. What is aliasing?**  
High-frequency content appears as false lower-frequency content after sampling.

**14. Why can circular DMA lose data?**  
DMA may overwrite a region before the consumer finishes.

**15. What makes a reusable driver?**  
Clear API, explicit dependencies, units, timeouts, errors, separation from app policy.

**16. When prefer a superloop?**  
Few simple activities with bounded non-blocking work and adequate timing.

**17. Queue vs semaphore vs mutex?**  
Queue transfers data; semaphore signals event / resource; mutex protects a shared resource.

**18. Why keep the ISR short?**  
A long ISR increases latency / jitter and risks missed deadlines.

**19. First checks for a missing I²C device?**  
Power, ground, pulls, address convention, pins, speed, waveform.

**20. How do you verify a timer rate?**  
Calculate from actual clock / registers and measure an output or diagnostic signal.

---

# Final engineering mindset

Good embedded engineering connects hardware facts, configuration, timing, data movement, application meaning, and fault handling. The goal is not merely to make a peripheral work once. Explain **why** it works, measure whether it meets requirements, and know how it behaves when something goes wrong.

| Question | Evidence |
|----------|----------|
| Is configuration correct? | Datasheet / reference manual, generated init, CubeMX clock / pinout. |
| Is timing correct? | Calculation plus scope / logic-analyzer measurement. |
| Is data valid? | Known input, raw codes, units, error bounds. |
| Can it keep up? | Bandwidth, worst-case processing time, overrun counters. |
| Can it recover? | Timeouts, status checks, bounded retries, visible error state. |
| Can others maintain it? | Small APIs, ownership rules, tests, version-controlled config. |

> **Final reminder.** Adapt code patterns to your exact STM32 family and board. Confirm details in the reference manual and datasheet before connecting hardware or relying on timing.
