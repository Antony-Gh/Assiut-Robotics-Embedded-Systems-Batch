# STM32 Session --- Complete Instructor's Guide

## CubeMX • IOC • Clocks • HAL • Interrupts • DMA • Timers • ADC • UART/SPI/I²C • Sensor Drivers • FreeRTOS

> **Purpose:** A detailed teaching and reference document for a
> practical STM32 session. It explains what each feature does, why it
> exists, when to use it, how to configure it, implementation patterns,
> common mistakes, debugging techniques, and engineering trade-offs.

> **Important family note:** STM32 is a large MCU family. Peripheral
> names, register layouts, timer clocks, DMA routing, ADC features, HAL
> APIs, interrupt priorities, and available middleware vary by device
> and Cube package. Always verify the exact part number against its
> datasheet, reference manual, and the HAL/LL documentation installed
> for that family.

------------------------------------------------------------------------

## Contents

1.  Learning goals and session plan
2.  STM32 development workflow: CubeIDE, CubeMX, IOC
3.  CubeMX terminology glossary
4.  Clock-tree configuration and clock calculations
5.  Peripheral enablement and parameter configuration
6.  HAL vs. LL vs. register-level programming
7.  Polling, interrupts, and DMA
8.  STM32 timers
9.  Integrated example: timer-triggered ADC + DMA
10. UART, SPI, and I²C
11. Choosing a communication protocol
12. Sensors, ADC, and real-time data acquisition
13. Buffers, throughput, and data flow
14. Reusable sensor drivers
15. Bare-metal superloop vs. RTOS
16. Essential FreeRTOS concepts
17. Debugging playbook and common traps
18. Practical lab sequence
19. Instructor preparation checklist
20. Knowledge-check questions and answers
21. Quick-reference formulas and decision tables

------------------------------------------------------------------------

# 1. Learning Goals and Session Plan

By the end of the session, students should be able to:

-   Explain the roles of STM32CubeMX, STM32CubeIDE, the `.ioc` file,
    generated code, HAL, and the debugger.
-   Configure GPIO, clocks, timers, ADC, DMA, and serial peripherals.
-   Explain the difference between polling, interrupts, and DMA.
-   Calculate a timer update rate from the actual timer input clock.
-   Design a stable sampling pipeline using a timer, ADC, DMA, and
    buffers.
-   Choose UART, SPI, or I²C based on device and application
    requirements.
-   Convert ADC readings into engineering units while accounting for
    practical limitations.
-   Write a sensor driver with a clear API, documented units, and error
    handling.
-   Decide when a superloop is sufficient and when an RTOS is worth its
    complexity.
-   Debug common peripheral configuration and data-flow failures
    systematically.

## Suggested six-hour evening schedule

  -----------------------------------------------------------------------
  Time                                Activity
  ----------------------------------- -----------------------------------
  18:00--18:30                        Arrival, toolchain checks, hardware
                                      and project setup

  18:30--19:10                        Part 1: CubeIDE, CubeMX, IOC,
                                      clocks, HAL/registers

  19:10--20:00                        Part 2: polling, interrupts, DMA,
                                      timers, ADC trigger chain

  20:00--20:30                        Prayer/rest break; adjust to local
                                      prayer times

  20:30--21:10                        Part 3: UART, SPI, I²C and protocol
                                      selection

  21:10--21:45                        Part 4: sensors, ADC, real-time
                                      acquisition

  21:45--22:20                        Part 5: reusable sensor drivers

  22:20--22:45                        Part 6: RTOS fundamentals

  22:45--23:15                        Integrated example and task mapping

  23:15--00:00                        Questions, debugging, optional
                                      deeper examples
  -----------------------------------------------------------------------

**Teaching pattern for every concept:** problem → explanation → design
decision → CubeMX setup → code → test → common mistakes.

Do not spend the entire session clicking through CubeMX. Make students
explain *why* a setting is needed and how they would verify that it
works.

------------------------------------------------------------------------

# 2. STM32 Development Workflow

## 2.1 STM32CubeMX

STM32CubeMX is a configuration and code-generation tool. It helps
configure:

-   MCU or board selection
-   Pin assignments and alternate functions
-   Clock tree and peripheral clocks
-   Peripheral modes and parameters
-   NVIC interrupt settings
-   DMA requests/channels/streams where supported
-   Middleware such as FreeRTOS, USB, or networking stacks, depending on
    the device/package
-   Code-generation options

CubeMX generates initialization code, but it does not automatically
write the whole application. The application behavior remains the
developer's responsibility.

## 2.2 STM32CubeIDE

STM32CubeIDE is an integrated development environment used to:

-   Edit C/C++ source files
-   Build and compile firmware
-   Link the application
-   Flash the target board
-   Debug with breakpoints, watch expressions, registers, and memory
    views
-   Inspect build errors and warnings

The exact tooling and menus can change between releases. The core
workflow remains similar.

## 2.3 What is the `.ioc` file?

The `.ioc` file stores the CubeMX project configuration, including items
such as:

-   Target MCU/board
-   Pin assignments
-   Clock settings
-   Enabled peripheral instances
-   Peripheral parameters
-   Interrupt and DMA configuration
-   Middleware and code-generation settings

It is **not** the compiled firmware. It is the configuration source used
to generate portions of the project.

### Typical project workflow

1.  Create a project and select the exact MCU or board.
2.  Configure pins and resolve pin conflicts.
3.  Configure the clock tree.
4.  Enable the required peripherals.
5.  Set peripheral parameters.
6.  Configure NVIC, DMA, middleware, and code-generation options where
    needed.
7.  Save the `.ioc` file.
8.  Generate code.
9.  Review generated initialization and warnings.
10. Add application logic in user-code sections or separate files.
11. Build, flash, test, and debug.
12. Keep the `.ioc` and source code together in version control.

### Code-generation tricks and cautions

-   Keep application logic out of generated regions that are not
    protected as user-code sections.
-   Prefer separate application and driver files for nontrivial
    projects.
-   After changing the `.ioc`, inspect the source-control diff. Do not
    assume regeneration changed only the setting you intended.
-   Save a known-good project before changing clocks or multiple
    peripherals.
-   If CubeMX reports pin conflicts, resolve them intentionally rather
    than repeatedly selecting random pins.
-   Make sure the selected MCU/package actually supports the peripheral
    and mode.
-   If a generated function is missing, check whether the peripheral is
    enabled, code generation completed, and the correct project
    configuration is active.

## 2.4 Typical generated project structure

Names vary by project and package, but a HAL project often contains:

``` text
Project/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── stm32xxxx_it.h
│   │   └── ...
│   └── Src/
│       ├── main.c
│       ├── stm32xxxx_it.c
│       ├── stm32xxxx_hal_msp.c
│       └── ...
├── Drivers/
│   ├── CMSIS/
│   └── STM32xxxx_HAL_Driver/
├── Middlewares/        # only if enabled/used
├── startup_*.s
├── linker script
└── Project.ioc
```

### Common generated functions

-   `SystemClock_Config()` --- configures system and bus clocks.
-   `MX_GPIO_Init()` --- configures GPIO pins.
-   `MX_USARTx_UART_Init()` --- initializes a UART/USART instance.
-   `MX_ADCx_Init()` --- initializes ADC configuration.
-   `MX_TIMx_Init()` --- initializes a timer.
-   `HAL_MspInit()` and peripheral `HAL_*_MspInit()` functions ---
    low-level peripheral support setup, such as GPIO alternate
    functions, clocks, and DMA linkage, depending on the generated
    project.

Do not assume every project has exactly the same functions. Inspect the
generated files for the target.

------------------------------------------------------------------------

# 3. CubeMX Terminology Glossary

  ----------------------------------------------------------------------------
  Term                    Meaning                    Practical use / caution
  ----------------------- -------------------------- -------------------------
  **ADC**                 Analog-to-Digital          Converts an analog
                          Converter                  voltage into a digital
                                                     code.

  **BSP**                 Board Support Package      Board-specific setup and
                                                     helper functions.

  **CAN / FDCAN**         Controller Area Network    Robust message-based
                          interface                  communication; FDCAN
                                                     supports CAN FD on
                                                     suitable devices.

  **CRC**                 Cyclic Redundancy Check    Calculates/checks a value
                          hardware                   used for error detection.

  **DAC**                 Digital-to-Analog          Converts a digital value
                          Converter                  to an analog output
                                                     voltage/current,
                                                     depending on hardware.

  **DMA**                 Direct Memory Access       Transfers data between
                                                     supported endpoints,
                                                     often peripheral
                                                     registers and RAM, with
                                                     less CPU copying.

  **EXTI**                External Interrupt/Event   Detects configured
                          controller                 external GPIO or internal
                                                     events and can request
                                                     CPU attention.

  **FPU**                 Floating-Point Unit        Hardware floating-point
                                                     support on supported
                                                     cores.

  **GPIO**                General-Purpose            Digital pins configured
                          Input/Output               as input, output, analog,
                                                     or alternate function.

  **HAL**                 Hardware Abstraction Layer Higher-level driver API
                                                     provided by STM32
                                                     software packages.

  **I²C**                 Inter-Integrated Circuit   Two-wire synchronous
                                                     addressed bus, typically
                                                     using pull-ups.

  **IWDG**                Independent Watchdog       Watchdog using an
                                                     independent clock source
                                                     on supported devices; can
                                                     reset the MCU if not
                                                     refreshed in time.

  **LL**                  Low-Layer driver library   More direct peripheral
                                                     APIs than HAL, with less
                                                     abstraction.

  **NVIC**                Nested Vectored Interrupt  Enables and prioritizes
                          Controller                 CPU interrupts.

  **PWR**                 Power control              Power scaling, low-power
                                                     modes, and power-related
                                                     configuration depending
                                                     on MCU.

  **RCC**                 Reset and Clock Control    Manages oscillators,
                                                     PLLs, bus clocks,
                                                     peripheral clocks, and
                                                     resets.

  **RTC**                 Real-Time Clock            Calendar/timekeeping or
                                                     low-power time base,
                                                     often using LSE or
                                                     another suitable clock.

  **SAI / I²S**           Digital audio interfaces   Audio data interfaces
                                                     available on selected
                                                     STM32 models.

  **SPI**                 Serial Peripheral          Synchronous serial
                          Interface                  interface, often used for
                                                     high-throughput sensors,
                                                     displays, and memory.

  **SYS / System Core**   System configuration       May include debug
                          category in CubeMX         configuration,
                                                     SysTick/time base, and
                                                     core-related settings;
                                                     exact options vary.

  **TIM**                 Timer peripheral           Timing, PWM, input
                                                     capture, output compare,
                                                     encoder functions, or
                                                     trigger generation
                                                     depending on timer type.

  **UART**                Universal Asynchronous     Asynchronous serial
                          Receiver/Transmitter       communication.

  **USART**               Universal                  Can support asynchronous
                          Synchronous/Asynchronous   UART-style mode and
                          Receiver/Transmitter       synchronous modes on
                                                     suitable devices.

  **USB**                 Universal Serial Bus       USB device/host/OTG
                                                     capability depending on
                                                     MCU and hardware.

  **WWDG**                Window Watchdog            Requires refresh within a
                                                     configured timing window;
                                                     too-early or too-late
                                                     refresh can trigger
                                                     reset.

  **FreeRTOS**            Real-time operating system Tasks, scheduling,
                                                     queues, semaphores,
                                                     mutexes, and software
                                                     timers.

  **Middleware**          Reusable software stack    Examples include RTOS,
                                                     USB, file systems,
                                                     networking, and protocol
                                                     stacks.

  **Pinout &              CubeMX configuration view  Assigns pins and
  Configuration**                                    enables/configures
                                                     peripherals.

  **Clock Configuration** Clock-tree editor          Selects clock sources,
                                                     PLL configuration, and
                                                     bus/peripheral clocks.

  **Peripheral            Per-instance settings      Baud rate, ADC sample
  Parameters**                                       time, timer period, SPI
                                                     mode, and similar
                                                     configuration.

  **NVIC Settings**       Interrupt configuration    Enables supported IRQs
                                                     and assigns priority
                                                     values.

  **DMA Settings**        DMA configuration          Selects
                                                     request/channel/stream,
                                                     direction, data width,
                                                     increment, mode, and
                                                     priority where supported.

  **GPIO Settings**       Pin electrical/function    Mode, pull-up/down,
                          settings                   output type, speed, and
                                                     alternate function.
  ----------------------------------------------------------------------------

## 3.1 Watchdogs: IWDG vs. WWDG

A watchdog helps recover from software that stops making expected
progress.

-   **IWDG:** independent clock source on supported STM32 devices.
    Commonly used as a robust system-health timeout.
-   **WWDG:** windowed refresh requirement. Refreshing too early can be
    as wrong as refreshing too late.

**Best practice:** Do not refresh a watchdog unconditionally from a
timer interrupt if the main application can be deadlocked. A watchdog is
useful only if refresh represents meaningful system health. Define what
"healthy" means and make sure the refresh strategy detects failures you
care about.

## 3.2 RTC vs. timer

-   **RTC:** date/timekeeping or low-power time base.
-   **General-purpose timer:** precise timing events, PWM, capture, or
    peripheral triggers.
-   **SysTick:** commonly provides an operating-system or HAL time base,
    but is not a substitute for every hardware timer use case.

------------------------------------------------------------------------

# 4. Clock-Tree Configuration Using the IOC File

## 4.1 Why clocks matter

Clock settings affect:

-   CPU execution speed
-   AHB/APB bus frequencies
-   Peripheral timing
-   UART baud-rate accuracy
-   Timer periods and PWM frequencies
-   ADC clock and conversion timing
-   USB or other peripherals with strict clock requirements
-   Power consumption and flash wait-state requirements

A peripheral can be configured correctly in its own parameter panel and
still behave incorrectly if the clock tree is wrong.

## 4.2 Common clock sources

Exact sources vary by MCU. Common names include:

-   **HSI:** internal high-speed oscillator
-   **HSE:** external high-speed clock/oscillator input
-   **LSI:** internal low-speed oscillator, often used by watchdogs or
    RTC-related functions
-   **LSE:** external low-speed crystal/clock, commonly used by RTC
-   **PLL:** phase-locked loop used to derive other frequencies from a
    source clock
-   **SYSCLK:** system clock source selected for the core/system
-   **HCLK:** AHB clock
-   **PCLK1/PCLK2:** APB bus clocks on many STM32 families
-   **Peripheral kernel clock:** the clock source selected for a
    particular peripheral, which may not simply equal PCLK

Not every STM32 has the same clock architecture or the same clock-source
options.

## 4.3 Clock configuration workflow

1.  Open the `.ioc` file.
2.  Go to **Clock Configuration**.
3.  Identify the board's actual clock source and frequency. Do not
    assume an external crystal exists just because the MCU supports HSE.
4.  Choose the system clock source.
5.  Configure PLL multipliers/dividers if needed and supported.
6.  Check SYSCLK, HCLK, APB clocks, peripheral kernel clocks, and
    limits.
7.  Check flash latency and voltage-scaling constraints where relevant.
8.  Resolve CubeMX warnings/errors.
9.  Save and regenerate code.
10. Inspect `SystemClock_Config()` and related clock setup.
11. Verify peripheral behavior with a known test.

## 4.4 Clock configuration tricks

-   **Never copy clock settings blindly** from a tutorial for a
    different board or MCU.
-   A board may use an external oscillator, a crystal, or an internal
    oscillator; verify the hardware schematic.
-   If UART output is garbled, verify the clock source and actual
    peripheral clock before changing the baud rate at random.
-   If PWM frequency is wrong, first verify the timer input clock,
    prescaler, ARR, counter mode, and repetition-counter behavior where
    applicable.
-   If a peripheral has a selectable kernel clock, inspect that
    selection rather than assuming it uses the APB clock.
-   Clock tree diagrams show derived frequencies, but the hardware and
    board configuration must match the diagram.
-   When changing system clocks, retest UART, timers, ADC, USB, and any
    peripheral with timing requirements.

## 4.5 Timer clock nuance

On many STM32 families, a timer connected to an APB bus may receive a
timer clock that differs from the APB peripheral clock when an APB
prescaler is not 1. A common arrangement doubles the timer clock in that
case, but **this is not universal across every STM32 family or timer
architecture**. Verify the reference manual and CubeMX clock diagram.

------------------------------------------------------------------------

# 5. Peripheral Enablement and Parameters

Enabling a peripheral allocates/configures the selected hardware
instance and may assign pins, clocks, interrupts, or DMA requests.
Parameters describe how it operates.

## 5.1 GPIO

Common GPIO settings:

-   **Input:** reads a digital level.
-   **Output push-pull:** actively drives high and low.
-   **Output open-drain:** actively pulls low and releases the line; an
    external or internal pull-up is generally needed.
-   **Alternate function:** connects the pin to a peripheral such as
    UART, SPI, I²C, timer PWM, or another supported function.
-   **Analog:** disables the normal digital input path for analog use,
    often used for ADC/DAC pins.
-   **Pull-up/pull-down:** biases an otherwise floating input.
-   **Output speed:** controls output transition characteristics/drive
    behavior; it is not a CPU clock setting.

### GPIO tricks

-   Confirm the pin exists on the package, not just on the MCU family.
-   Check the board schematic: an LED may be active-low, connected to a
    different pin, or shared with another function.
-   Floating inputs can produce random readings. Choose a suitable
    pull-up/down or external circuit.
-   Do not drive loads beyond pin current limits; use a
    transistor/MOSFET/driver for motors, relays, high-current LEDs, and
    similar loads.
-   On many MCUs, a pin's alternate function must match the selected
    peripheral signal.
-   Avoid reconfiguring a pin that is already assigned to another
    peripheral.

## 5.2 UART parameters

-   Baud rate
-   Word length
-   Parity
-   Stop bits
-   Oversampling, where available
-   Hardware flow control, if needed
-   TX/RX pin mapping

The transmitter and receiver must agree on framing and use compatible
logic levels. UART logic levels are not the same as RS-232 voltage
levels; use the correct interface hardware.

## 5.3 I²C parameters

-   Timing/clock speed
-   Addressing mode
-   Own address for target/slave mode
-   Analog/digital filter options where supported
-   SDA/SCL pin assignment

Check pull-ups, bus capacitance, device addresses, voltage levels, and
the device's maximum clock rate.

## 5.4 SPI parameters

-   Controller or peripheral role
-   Full-duplex/half-duplex/simplex mode
-   Data width
-   CPOL and CPHA
-   Bit order
-   NSS/chip-select mode
-   Clock prescaler
-   NSS pulse or related options where supported

The SPI mode must match the peripheral datasheet. A wrong CPOL/CPHA
combination can produce data that looks almost plausible but is
incorrect.

## 5.5 ADC parameters

-   Resolution
-   Input channel and rank/sequence
-   Sample time
-   Trigger source and edge
-   Continuous/discontinuous mode
-   Scan mode
-   Data alignment
-   DMA configuration
-   Oversampling or calibration options where supported

**Important:** A pin connected to an ADC channel generally needs the
correct analog GPIO mode. Input voltage must stay within the device's
permitted range.

## 5.6 Timer parameters

-   Prescaler (PSC)
-   Counter mode
-   Auto-reload value (ARR/period)
-   Clock division
-   PWM channel/polarity/pulse
-   Trigger output selection
-   Interrupt and DMA settings
-   Repetition counter on timers that have one

## 5.7 NVIC and DMA

**NVIC settings** enable interrupt sources and assign priority levels.
Interrupt priority behavior and numbering conventions depend on the core
and configuration.

**DMA settings** may include:

-   Request/channel/stream mapping
-   Direction
-   Peripheral and memory data widths
-   Peripheral/memory increment
-   Normal or circular mode
-   Priority
-   FIFO/burst options on controllers that support them

The exact options vary by family. A valid DMA setup must match the
peripheral request, memory region, transfer width, and the expected data
format.

------------------------------------------------------------------------

# 6. HAL vs. LL vs. Register-Level Programming

## 6.1 HAL

The Hardware Abstraction Layer offers higher-level functions and handle
structures. It can improve readability and speed development.

Example:

``` c
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
```

Benefits:

-   Easier-to-read common operations
-   Consistent status return values for many APIs
-   Often easier to maintain in a team
-   Fits CubeMX-generated projects
-   Can ease migration within supported families, though it is not
    universally source-compatible

Trade-offs:

-   Function calls and state handling may add overhead
-   Some hardware details are hidden
-   You still need to understand the peripheral and check return values

## 6.2 LL (Low-Layer)

LL libraries expose more direct operations than HAL and may be useful
when you need tighter control while still using ST-provided definitions
and helpers.

Use LL when the project benefits from a lower-level style but you prefer
not to hand-code every register bit.

## 6.3 Register-level

Illustrative BSRR write to reset PC13 on compatible STM32 GPIO hardware:

``` c
GPIOC->BSRR = (1U << (13U + 16U));
```

This assumes PC13 exists, the GPIO clock is enabled, and the pin is
configured as an output. Register layouts and semantics must be verified
for the exact device.

Benefits:

-   Direct control over register fields
-   Can be efficient for a carefully chosen critical operation
-   Excellent for understanding hardware

Trade-offs:

-   Easy to make subtle mistakes
-   More tightly coupled to the MCU
-   Requires reference-manual knowledge
-   Can make code harder to review and port

## 6.4 Which should students use?

  -----------------------------------------------------------------------
  Situation                           Sensible starting choice
  ----------------------------------- -----------------------------------
  Standard application feature        HAL

  Need lower-level control with ST    LL
  helpers                             

  Learning the peripheral register    Register-level study
  model                               

  Performance-critical path after     Profile first; optimize only if
  measurement                         needed

  Team project requiring              Use a consistent documented
  maintainability                     abstraction
  -----------------------------------------------------------------------

Do not teach that register-level code is automatically faster or that
HAL is automatically slow. Measure the real bottleneck and choose the
lowest level that solves the problem safely and maintainably.

------------------------------------------------------------------------

# 7. Polling, Interrupts, and DMA

## 7.1 Polling

The CPU checks a peripheral's status or calls a function that waits for
completion.

Good for:

-   Infrequent sensor reads
-   Simple prototypes
-   Short bounded operations
-   Cases where simplicity is more important than CPU efficiency

Weaknesses:

-   Can waste CPU cycles
-   Blocking calls can delay other work
-   Timing can vary if the loop does different amounts of work
-   Long waits can make a superloop unresponsive

Example pattern:

``` c
if (HAL_ADC_Start(&hadc1) == HAL_OK)
{
    if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
    {
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        // Process raw.
    }
    (void)HAL_ADC_Stop(&hadc1);
}
```

This is a generic HAL pattern; exact setup and required API sequence
depend on ADC configuration and family.

## 7.2 Interrupts

A hardware event requests CPU attention. The CPU pauses normal
execution, runs an interrupt handler, then resumes.

Good for:

-   Button edges
-   Timer update events
-   UART receive events
-   Peripheral completion/error notifications
-   Low-latency event handling

Rules:

-   Keep interrupt service routines (ISRs) short.
-   Avoid blocking delays and long polling loops in an ISR.
-   Avoid heavy formatting or `printf` in an ISR.
-   Share data with main/task code carefully; `volatile` alone does not
    make multi-step access atomic or provide complete synchronization.
-   Understand interrupt priority and nesting.
-   Use ISR-safe RTOS APIs only where permitted by the FreeRTOS port and
    interrupt-priority rules.

Example timer callback:

``` c
volatile uint32_t tick_count = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        tick_count++;
    }
}
```

For real applications, consider overflow, atomic access, and whether a
flag/notification is a better design than a shared counter.

## 7.3 DMA

DMA transfers data between supported peripheral/memory endpoints without
requiring the CPU to copy each item.

Good for:

-   ADC sample blocks
-   UART receive streams
-   SPI transfers
-   Audio/data streams
-   Repeated high-rate peripheral activity

DMA does **not** perform sensor interpretation or application
processing. It moves bytes/halfwords/words according to the
configuration.

## 7.4 How to choose

  -----------------------------------------------------------------------
  Requirement                         Likely approach
  ----------------------------------- -----------------------------------
  Read a sensor once every few        Polling or a simple
  seconds                             interrupt-driven method

  React to a button edge              EXTI interrupt, with debounce

  Run an action periodically          Hardware timer interrupt or
                                      timer-triggered peripheral event

  Generate PWM                        Timer hardware PWM

  Collect analog samples at a stable  Timer-triggered ADC
  rate                                

  Collect a block of ADC samples      Timer-triggered ADC + DMA
  efficiently                         

  Receive a continuous UART stream    UART DMA or interrupt reception
                                      with proper buffering

  Tiny, low-rate transfer             Polling may be simpler and
                                      sufficient
  -----------------------------------------------------------------------

## 7.5 DMA modes and buffer ownership

-   **Normal mode:** transfers a finite block and then stops or
    completes.
-   **Circular mode:** wraps around and keeps transferring.
-   **Half-transfer callback:** signals that the first half of a
    circular buffer has been filled.
-   **Transfer-complete callback:** signals that the full configured
    transfer has completed; in circular mode, DMA may wrap and begin
    overwriting the buffer.

The CPU must process a region before DMA overwrites it. A callback does
not magically freeze the buffer.

### DMA tricks

-   Verify the DMA request/channel/stream for the exact MCU.
-   Check peripheral and memory widths. A 16-bit ADC result stored in a
    32-bit buffer may be valid in some configurations but wastes space
    and must match the configured transfer width.
-   For peripheral-to-memory ADC DMA, peripheral increment is typically
    disabled and memory increment enabled.
-   For memory-to-peripheral UART/SPI transmit, the direction and
    increment settings differ.
-   Check whether the target RAM region is accessible to the DMA
    controller.
-   On cache-equipped cores, handle cache coherency according to the
    MCU/core documentation.
-   Do not use a local stack buffer for an asynchronous DMA transfer if
    the function returns before the transfer ends.
-   Do not modify a transmit buffer while DMA is still reading it.
-   Track errors and overrun conditions, not only successful callbacks.

------------------------------------------------------------------------

# 8. STM32 Timers

## 8.1 What is a timer?

A timer counts clock events. Depending on its type and configuration, it
can:

-   Generate periodic update events
-   Trigger interrupts
-   Generate PWM
-   Measure input pulse width or frequency
-   Count encoder transitions
-   Generate output-compare events
-   Trigger another peripheral such as ADC
-   Coordinate timing without software needing to act on every event

## 8.2 Timer categories

Common categories include:

-   **Basic timers:** often intended for time-base or trigger use.
-   **General-purpose timers:** commonly provide base timing, input
    capture, output compare, and PWM.
-   **Advanced-control timers:** may provide complementary PWM outputs,
    dead-time insertion, break inputs, and other motor-control-oriented
    features.
-   **Low-power timers:** available on selected devices for low-power
    timing.
-   **SysTick:** core timer often used for a system time base; separate
    from general-purpose TIM peripherals.
-   **Watchdogs:** safety timeout peripherals, not general-purpose
    timing substitutes.

Exact capabilities depend on the specific MCU.

## 8.3 Timer math

For a typical up-counting timer:

\[ f\_{`\text{counter}`{=tex}} = `\frac{f_{\text{TIM}}}{PSC+1}`{=tex} \]

\[ f\_{`\text{update}`{=tex}} =
`\frac{f_{\text{TIM}}}{(PSC+1)(ARR+1)}`{=tex} \]

Where:

-   (f\_{`\text{TIM}`{=tex}}) is the timer input clock
-   `PSC` is the prescaler register value
-   `ARR` is the auto-reload register value

Example: if the timer input clock is 72 MHz, `PSC = 71`, and
`ARR = 999`:

-   Counter clock = 72 MHz / 72 = 1 MHz
-   Counter tick = 1 µs
-   Update period = 1,000 ticks = 1 ms
-   Update frequency = 1 kHz

These formulas assume ordinary up-counting and no extra
repetition-counter or clock-division effects that change the result.

### Timer calculation tricks

-   Register values usually divide by **value + 1**.
-   Find the timer's actual input clock first. Do not automatically use
    SYSCLK.
-   Check APB prescaler behavior for the exact family.
-   Center-aligned counting can change the frequency relationship.
-   Advanced timers may have repetition counters.
-   Verify that the selected ARR/PSC values fit the timer's counter
    width.
-   If a period is off by a factor of two, inspect the clock tree and
    timer clock rules before changing values randomly.

## 8.4 PWM basics

For common edge-aligned up-counting PWM, the period is often based on
`PSC` and `ARR`, and duty cycle is set by the compare value (`CCR`)
relative to the period. Exact equations depend on PWM mode and counter
configuration.

Conceptually:

-   Frequency determines how often the waveform repeats.
-   Duty cycle determines the proportion of the period spent active.
-   Polarity determines which electrical level is considered active.
-   Output compare/PWM requires the correct alternate-function pin and
    timer channel.

PWM is commonly used for LED dimming, motor control inputs, and other
periodic control signals. Do not directly drive a motor from an MCU pin.

## 8.5 Practical timer interrupt

CubeMX setup:

1.  Enable a suitable timer.
2.  Choose the counter clock/prescaler and period.
3.  Enable the timer interrupt in NVIC.
4.  Generate code.
5.  Start the timer in interrupt mode.
6.  Implement a short callback.
7.  Test the actual period with a pin toggle, logic analyzer, or
    oscilloscope where possible.

Example:

``` c
if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
{
    Error_Handler();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        // Keep this short.
        // Set a flag, increment a counter, or notify a task.
    }
}
```

This assumes correct timer initialization and IRQ routing through the
HAL handler.

### Timer pitfalls

-   Starting the timer without enabling the relevant IRQ when an
    interrupt is expected.
-   Forgetting to call the correct HAL IRQ handler from the generated
    interrupt handler.
-   Doing blocking UART output in the callback.
-   Assuming the timer input clock equals the CPU clock.
-   Forgetting that an LED may be active-low.
-   Using a timer channel pin that is not configured for the timer's
    alternate function.
-   Reconfiguring a timer already used by HAL tick or another subsystem
    without understanding the conflict.

------------------------------------------------------------------------

# 9. Integrated Example: Timer-Triggered ADC + DMA

This is one of the most important examples because it connects clocks,
timers, ADC, DMA, interrupts, buffers, and application processing.

## 9.1 Goal

Collect analog measurements at a predictable rate while minimizing CPU
work for each sample.

## 9.2 Data path

``` text
Timer event
    ↓ hardware trigger
ADC conversion
    ↓ conversion result
DMA transfer
    ↓
RAM sample buffer
    ↓ half/full transfer notification
Application processes completed block
    ↓
Filtering / scaling / logging / transmission
```

The timer controls when sampling occurs. The ADC converts the signal.
DMA moves results into RAM. The CPU handles completed blocks and
application logic.

## 9.3 Design decisions

Before configuring CubeMX, answer:

1.  What sample rate is required?
2.  What is the maximum signal frequency of interest?
3.  How many ADC channels are needed?
4.  What ADC resolution and sample time are appropriate?
5.  Can the ADC conversion complete before the next trigger?
6.  What buffer size balances latency and processing efficiency?
7.  Can the CPU process each block before DMA overwrites it?
8.  Does the MCU require cache maintenance for DMA buffers?
9.  How will errors, overruns, and missed processing deadlines be
    detected?

## 9.4 Configuration outline

1.  Set the analog input pin to analog mode.
2.  Enable the ADC and choose channel/rank(s).
3.  Choose a suitable ADC sample time and resolution.
4.  Select an external trigger from a timer event supported by that ADC.
5.  Configure DMA in the appropriate direction and data width.
6.  Enable memory increment.
7.  Choose normal or circular DMA mode.
8.  Configure the timer prescaler and ARR for the desired trigger
    frequency.
9.  Enable required DMA/ADC interrupts through the generated
    configuration.
10. Start the acquisition according to the HAL family's documented
    sequence.
11. Use half/full callbacks or another supported mechanism to identify
    completed regions.
12. Process completed data outside the ISR where practical.

**Family warning:** Timer-to-ADC trigger routing, ADC start functions,
DMA request setup, and callback names vary. The exact implementation
must match the target MCU and HAL version.

## 9.5 Buffer example

A conceptual circular buffer:

``` c
#define ADC_BUFFER_LENGTH  256U

uint16_t adc_buffer[ADC_BUFFER_LENGTH];
```

The buffer can be processed in two halves if the DMA configuration and
callback behavior support this arrangement. The first half and second
half are not safe to process at arbitrary times; coordinate based on DMA
events.

## 9.6 Sample-rate calculation

If the timer update/trigger frequency is 1 kHz, the nominal trigger
interval is 1 ms. For a single ADC channel, that means one conversion
trigger every millisecond if the ADC trigger configuration and
conversion timing support it.

For scan sequences with multiple channels, distinguish:

-   Trigger frequency
-   Per-channel sample rate
-   Total conversion sequence duration
-   DMA item rate

Do not assume each channel is sampled at the full trigger rate if the
sequence/trigger model differs.

## 9.7 Aliasing and analog considerations

Sampling must be chosen based on the signal's frequency content. A
sampled system can misrepresent frequencies above its Nyquist limit.
Practical anti-alias filtering is often required for analog signals.

Also consider:

-   ADC input impedance and source impedance
-   Sample-and-hold acquisition time
-   Reference voltage quality
-   Grounding and analog noise
-   Input protection and voltage limits
-   ADC calibration where supported

## 9.8 How to verify the example

-   Start with a stable known voltage or potentiometer.
-   Confirm the buffer changes as expected.
-   Check minimum/maximum ADC codes.
-   Use a timer output pin or diagnostic GPIO to verify the timing if
    possible.
-   Compare expected and measured sample rates.
-   Watch for ADC overrun, DMA errors, or missed blocks.
-   Test at a low rate first, then increase the rate gradually.
-   Confirm the CPU processing time is less than the time available
    before the buffer region is reused.

------------------------------------------------------------------------

# 10. Communication Protocols

# 10.1 UART / USART

UART is asynchronous serial communication. Common signals are TX and RX
plus a shared ground.

Important settings:

-   Baud rate
-   Word length
-   Parity
-   Stop bits
-   Flow control
-   TX/RX pins
-   Polling, interrupt, or DMA transfer method

Example blocking transmission:

``` c
uint8_t message[] = "Hello STM32\r\n";

HAL_UART_Transmit(
    &huart1,
    message,
    sizeof(message) - 1U,
    100U
);
```

This call blocks until the transfer completes or times out. The exact
meaning of timeout and return status is defined by the HAL API.

### UART practical tips

-   Cross TX to the other device's RX and RX to TX if bidirectional
    communication is needed.
-   Connect ground between devices.
-   Verify logic voltage compatibility.
-   A UART logic interface is not the same as RS-232 voltage levels.
-   Check baud rate, parity, word length and stop bits at both ends.
-   If output is garbled, verify clocks and baud configuration first.
-   For continuous reception, plan buffering and overflow handling.
-   Avoid `printf` from high-priority ISRs unless the implementation is
    specifically designed for it.
-   For a long transmit, DMA/interrupt-based transfer can avoid blocking
    the main loop.

## 10.2 I²C

I²C is a synchronous, addressed bus that commonly uses SDA and SCL.

Key concepts:

-   START and STOP conditions
-   7-bit or, less commonly, 10-bit addresses
-   ACK/NACK
-   Register addresses and read/write sequences
-   Pull-up resistors
-   Clock speed and bus capacitance
-   Bus recovery and error handling

### I²C practical tips

-   Verify pull-ups exist and are appropriate for the bus voltage and
    capacitance.
-   Multiple breakout boards may each contain pull-ups; too many
    parallel pull-ups can make the effective resistance too low.
-   Check that all devices support the selected bus speed.
-   Verify voltage compatibility; not every sensor module is 5 V
    tolerant.
-   Confirm whether a datasheet gives a 7-bit address or an 8-bit
    read/write-form address.
-   Some STM32 HAL APIs expect the 7-bit address shifted into a
    particular position; follow the specific API documentation rather
    than assuming.
-   Use an I²C scanner only as a diagnostic aid, not as proof that a
    device works correctly.
-   If the bus is stuck low, inspect wiring, pull-ups, device power, and
    whether a device is holding the line.
-   Use a logic analyzer to inspect START, address, ACK/NACK and data
    when available.

## 10.3 SPI

SPI commonly uses:

-   **SCK:** serial clock
-   **MOSI:** controller-to-peripheral data
-   **MISO:** peripheral-to-controller data
-   **CS/NSS:** chip select

Important settings:

-   Controller/peripheral role
-   CPOL
-   CPHA
-   Data width
-   Bit order
-   Clock prescaler
-   Chip-select behavior
-   Full-duplex/half-duplex mode

### SPI practical tips

-   CPOL and CPHA must match the peripheral's timing requirements.
-   Check maximum SPI clock frequency in the sensor/display/memory
    datasheet.
-   Verify chip select is active at the correct time and polarity.
-   If receiving all `0xFF` or `0x00`, check MISO wiring, CS, device
    power, SPI mode, and whether the device is responding.
-   Some devices require a dummy byte/clock cycle before data becomes
    valid.
-   Check whether the device expects multi-byte reads to set an address
    auto-increment bit.
-   SPI usually has more wires than I²C but can provide higher
    throughput in suitable implementations.

------------------------------------------------------------------------

# 11. Choosing a Communication Protocol

  ---------------------------------------------------------------------------
  Consideration     UART              I²C                  SPI
  ----------------- ----------------- -------------------- ------------------
  Clocking          Asynchronous      Synchronous          Synchronous

  Typical signal    TX/RX             SDA/SCL              SCK/MOSI/MISO/CS
  lines                                                    

  Device addressing Not usually a     Yes                  Usually chip
                    native shared                          select
                    addressed bus                          

  Multiple          Extra             Shared addressed bus Often one CS per
  peripherals       design/logic may                       device
                    be needed                              

  Common use        Debug console,    Low/moderate-speed   Displays, flash,
                    GPS, serial       sensors              high-throughput
                    modules                                sensors

  Main trade-offs   Framing and baud  Pull-ups, addresses, Pin count, CS
                    agreement         bus capacitance      management, mode
                                                           matching
  ---------------------------------------------------------------------------

## Decision exercise

-   GPS module sends position data → UART is common.
-   Several compatible low-rate temperature sensors → I²C may reduce
    wiring.
-   Display requires frequent updates → SPI may be a good choice if
    supported.
-   External flash memory → SPI or a dedicated memory interface,
    depending on bandwidth needs.

**Do not select a protocol by slogan.** Check the device datasheet,
required data rate, latency, wiring, topology, power, electrical levels,
MCU pin availability, and software complexity.

------------------------------------------------------------------------

# 12. Sensors, ADC, and Real-Time Data Acquisition

## 12.1 Analog vs. digital sensors

  -----------------------------------------------------------------------
  Sensor output           Example                 Typical MCU interface
  ----------------------- ----------------------- -----------------------
  Analog voltage          Potentiometer, analog   ADC
                          temperature sensor      

  Digital bus             Digital temperature     I²C or SPI
                          sensor, IMU             

  Serial stream           GPS receiver            UART

  Pulse/frequency         Flow meter, speed       Timer input
                          sensor                  capture/counter or GPIO
                                                  interrupt
  -----------------------------------------------------------------------

A digital sensor may contain an internal ADC. "Digital" describes the
interface presented to the MCU, not necessarily the physics of sensing.

## 12.2 ADC fundamentals

An ADC maps a voltage into a digital code. For an ideal N-bit ADC:

-   Number of possible codes: (2\^N)
-   Maximum code: (2\^N - 1)

Approximate ideal relationship:

\[ `\text{ADC code}`{=tex}
`\approx `{=tex}`\frac{V_{\text{in}}}{V_{\text{ref}}}`{=tex}(2\^N-1) \]

\[ V\_{`\text{in}`{=tex}}
`\approx `{=tex}`\frac{\text{ADC code}}{2^N-1}`{=tex}V\_{`\text{ref}`{=tex}}
\]

For a 12-bit ADC, the codes range from 0 to 4095. If Vref is 3.3 V, a
voltage near 1.65 V should produce a code near half-scale in an ideal
system.

Actual readings are affected by:

-   Reference voltage accuracy and noise
-   ADC offset/gain error and nonlinearity
-   Source impedance and acquisition time
-   Noise and grounding
-   Input range and pin limits
-   Calibration and temperature
-   Sensor output impedance and filtering

### ADC resolution vs. accuracy

Resolution is the number of discrete output codes. Accuracy is how close
the measured value is to the true value. A 12-bit ADC is not
automatically accurate to 1 part in 4096.

### ADC input safety

Never assume an STM32 ADC pin tolerates 5 V. Check the exact datasheet's
analog-input limits and absolute maximum ratings. Use a divider, buffer,
or protection circuit where appropriate.

## 12.3 Sample rate, jitter, and latency

-   **Sample rate:** number of samples per second.
-   **Sample interval:** nominal time between samples.
-   **Jitter:** variation in sample timing.
-   **Latency:** time between acquisition and the data being
    used/delivered.
-   **Throughput:** data processed or transferred per unit time.
-   **Overrun/data loss:** new data arrives before previous data is
    handled.

A software delay loop does not guarantee precise sampling. Interrupts,
blocking functions, and variable code execution can change the sample
interval. A hardware timer trigger is often a better way to establish a
stable sampling schedule.

## 12.4 Real-time does not simply mean fast

A real-time system is one where correctness depends partly on meeting
timing constraints. A fast average response is not enough if a deadline
is occasionally missed.

Ask:

-   What is the deadline?
-   What is the worst-case processing time?
-   What happens if data arrives faster than it can be processed?
-   Is the sampling schedule deterministic enough?
-   Can a blocking communication call delay acquisition or processing?

------------------------------------------------------------------------

# 13. Buffers, Throughput, and Data Flow

## 13.1 Why use buffers?

A buffer temporarily stores data so acquisition, processing, and
transmission can proceed in blocks instead of requiring every sample to
be handled immediately.

Example pipeline:

``` text
Sensor / ADC → acquisition buffer → processing → output buffer → UART/SPI/USB
```

Each stage may have a different execution time. Buffers absorb temporary
timing differences but do not solve a permanent throughput mismatch.

## 13.2 Circular DMA buffers

A circular DMA buffer is reused repeatedly. Half-transfer and
transfer-complete events can tell the CPU which half has been filled,
depending on the DMA/peripheral configuration.

Rules:

-   Process only the region known to be complete.
-   Finish processing before DMA wraps and overwrites that region.
-   Avoid slow operations inside DMA callbacks.
-   Track dropped blocks and overruns.
-   Define who owns each buffer region at each moment.
-   Be mindful of data cache on cache-equipped devices.

## 13.3 Throughput calculation

For a stream with sample rate (f_s), (C) channels, and (B) bytes per
sample:

\[ R\_{`\text{raw}`{=tex}} = f_s `\times `{=tex}C `\times `{=tex}B \]

Example: 10,000 samples/s × 2 channels × 2 bytes/sample = 40,000 bytes/s
of raw sample payload, before protocol framing or metadata.

For a buffer with (N) samples:

\[ T\_{`\text{buffer}`{=tex}} = `\frac{N}{f_s}`{=tex} \]

If a buffer contains 256 samples at 1,000 samples/s, the buffer spans
0.256 s for a single sample stream. For multi-channel scans, clarify
whether N refers to total conversion results or samples per channel.

## 13.4 Buffer-size trade-off

Larger buffers:

-   Reduce notification frequency and per-block overhead
-   May improve throughput efficiency
-   Increase latency and RAM use

Smaller buffers:

-   Reduce latency
-   Use less RAM
-   Increase callback/processing frequency and overhead

Choose a size based on sample rate, processing time, RAM, and latency
requirements.

------------------------------------------------------------------------

# 14. Reusable Sensor Drivers

## 14.1 What is a driver?

A sensor driver encapsulates the operations needed to communicate with
and operate a sensor. It gives the application a clean API so
application code does not need to know every register address and
transaction detail.

A driver may provide:

-   Initialization and device identity checks
-   Configuration of measurement range, rate, filters, or mode
-   Measurement reads
-   Conversion from raw values to engineering units
-   Error/status reporting
-   Timeout and recovery behavior

## 14.2 Example structure

``` text
Drivers/
└── Sensors/
    └── sensor_name/
        ├── sensor_name.h
        └── sensor_name.c

Application/
├── app.h
└── app.c
```

This is an example layout, not a requirement. The goal is separation of
concerns.

## 14.3 Header/API example

``` c
#ifndef SENSOR_NAME_H
#define SENSOR_NAME_H

#include <stdint.h>
#include "stm32f1xx_hal.h"  /* Replace with the target family header. */

HAL_StatusTypeDef Sensor_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef Sensor_Read(I2C_HandleTypeDef *hi2c,
                              float *measurement);

#endif
```

What the API means:

-   Include guards prevent repeated inclusion.
-   Function declarations define the public interface.
-   Passing a peripheral handle avoids unnecessarily hiding one global
    peripheral inside the driver.
-   The output pointer returns a measurement.
-   The return status communicates success/failure.
-   Units and valid ranges must be documented.
-   The family-specific HAL header must match the target MCU package.

## 14.4 Application usage

``` c
float temperature_c;

if (Sensor_Read(&hi2c1, &temperature_c) == HAL_OK)
{
    /* Use temperature_c; document its units. */
}
else
{
    /* Report, retry, or recover based on application policy. */
}
```

The application should not duplicate all of the sensor's register-read
logic.

## 14.5 Design principles for reusable drivers

1.  Keep sensor register details inside the driver.
2.  Expose a small, clear public API.
3.  Document units, valid ranges, and timing.
4.  Return meaningful errors rather than silently ignoring failures.
5.  Avoid unnecessary application-specific global variables.
6.  Pass transport handles or interfaces explicitly where practical.
7.  Separate initialization, configuration, and measurement reading.
8.  Follow the datasheet's register and timing requirements.
9.  Use timeouts so a failed bus/device cannot hang the system
    indefinitely.
10. Document whether calls block.
11. Avoid doing slow printing inside the driver; return status/data and
    let the application decide how to report it.
12. Test error paths, not only successful reads.
13. Consider a transport abstraction if the driver should support more
    than one MCU family or bus implementation.
14. Avoid creating a universal framework for a one-off project unless it
    solves a real maintenance problem.

## 14.6 Common driver mistakes

-   Using the wrong register address or byte order
-   Misinterpreting signed data
-   Forgetting scaling, offset, or units
-   Not checking a HAL return status
-   Using a timeout that is too short or effectively infinite
-   Assuming a device is present without checking
-   Mixing sensor logic with display/logging/application policy
-   Keeping a pointer to a temporary buffer
-   Assuming I²C addresses are represented the same way in every API

------------------------------------------------------------------------

# 15. Bare-Metal Superloop vs. RTOS

## 15.1 Bare-metal superloop

A basic program may look like:

``` c
while (1)
{
    ReadSensors();
    ProcessData();
    UpdateDisplay();
    SendData();
}
```

This is often a good design for a small, predictable application when
each operation is short and non-blocking.

Potential problems:

-   A blocking sensor or UART call delays the rest of the loop.
-   Execution time varies depending on branches and data.
-   Scheduling responsibilities become scattered as features grow.
-   Timing requirements can become hard to reason about.

## 15.2 What an RTOS provides

An RTOS provides a scheduler and synchronization primitives that
organize concurrent activities into tasks. On a single-core MCU, tasks
usually share CPU time; they are not necessarily running simultaneously.

An RTOS can help with:

-   Independent activities with different timing needs
-   Blocking I/O without freezing every other activity
-   Queues and message passing
-   Resource synchronization
-   Structured timing and task organization

It also adds costs:

-   RAM for task stacks and control structures
-   CPU overhead and context switching
-   More complex priority and synchronization behavior
-   Need for stack sizing, deadlock analysis, and debugging discipline

## 15.3 When to use which?

Use a superloop when:

-   The application is small.
-   Activities are easy to schedule.
-   Functions are short/non-blocking.
-   A minimal and auditable design is valuable.

Consider an RTOS when:

-   Several independent activities have different timing needs.
-   Blocking calls complicate responsiveness.
-   Inter-task communication is useful.
-   Complexity is high enough that explicit task structure improves
    maintenance.

An RTOS is not automatically better. Choose based on requirements, not
fashion.

------------------------------------------------------------------------

# 16. Essential FreeRTOS Concepts

## 16.1 Task

A task is a schedulable execution context with its own stack and state.
It is not merely a function called repeatedly by the main loop.

Example task structure (conceptual):

``` c
void SensorTask(void *argument)
{
    for (;;)
    {
        /* Acquire or receive sensor data. */
        /* Delay/block/await a notification as appropriate. */
    }
}
```

A real task must be created using the API and configuration appropriate
to the FreeRTOS version and project.

## 16.2 Scheduler and priority

The scheduler chooses which ready task runs according to the scheduling
policy and priorities. Priorities should reflect real timing
requirements, not simply which task seems important.

Beware of:

-   Starvation of low-priority tasks
-   Priority inversion around shared resources
-   Tasks that never block/yield when they should
-   Excessive high-priority work
-   Long critical sections

## 16.3 Queue

A queue transfers messages/data between tasks safely.

Example architecture:

``` text
Acquisition task → queue of samples/records → processing task
Processing task  → output queue            → communication task
```

Choose queue length based on the rate of production, rate of
consumption, acceptable latency, and RAM. A queue that is always full
indicates the consumer may be too slow or the capacity/architecture is
wrong.

## 16.4 Semaphore

A semaphore is commonly used to signal an event or coordinate access.
Binary/counting semaphores have different use cases.

For an interrupt-to-task handoff, use an ISR-safe API and follow the
port's priority restrictions.

## 16.5 Mutex

A mutex protects a shared resource against simultaneous access. It can
provide priority inheritance in common FreeRTOS configurations, unlike a
simple binary semaphore. Do not use a mutex from an ISR.

Examples of shared resources:

-   A peripheral bus shared by tasks
-   A shared data structure
-   A device interface that must be used by only one task at a time

## 16.6 Software timer

A FreeRTOS software timer schedules a callback relative to RTOS ticks.
It is useful for noncritical periodic actions, but it is not a
replacement for a hardware timer when precise sampling or waveform
generation is required.

## 16.7 Task notifications

Task notifications provide a lightweight way to signal a task or pass a
small amount of event/value information. They can be simpler than a
queue when the communication pattern is a single task receiving a
notification.

## 16.8 Interrupt and RTOS rules

-   Do not call ordinary task APIs from an ISR unless an ISR-specific
    variant is documented.
-   Interrupt priority must be compatible with the FreeRTOS port's
    syscall priority rules.
-   Keep ISR work short and defer processing to tasks.
-   Never assume `volatile` alone solves synchronization.
-   Avoid blocking while holding a mutex.
-   Keep task stack sizes appropriate and monitor stack high-water marks
    where supported.
-   Check return values for task creation, queue operations, and other
    APIs.
-   Make sure queues can handle expected bursts.

## 16.9 Example sensor-monitoring design

-   Hardware timer triggers ADC.
-   DMA fills a sample buffer.
-   DMA callback signals an acquisition/processing task.
-   Processing task filters/scales samples.
-   A queue passes measurements to the communication task.
-   Communication task transmits results without blocking the
    acquisition timing path.

This design still requires buffer ownership, correct ISR-safe signaling,
adequate queue capacity, and enough processing throughput.

------------------------------------------------------------------------

# 17. Debugging Playbook and Common Traps

## 17.1 The systematic order

When a peripheral does not work, debug from the physical layer upward:

1.  **Hardware:** power, ground, cable, correct pin, voltage level,
    soldering.
2.  **Part identity:** exact MCU and board variant.
3.  **Pin mapping:** correct package pin and alternate function.
4.  **Clock:** source, frequency, peripheral kernel clock.
5.  **Configuration:** generated initialization and parameters.
6.  **Interrupt/DMA:** enablement, priority, request routing, handler.
7.  **Application sequence:** start calls, callback assumptions, state
    transitions.
8.  **Data interpretation:** widths, signedness, byte order, units,
    scaling.
9.  **Timing/throughput:** blocking calls, buffer overruns, missed
    deadlines.
10. **Instrumentation:** status codes, watch variables, logic analyzer,
    scope, serial logs.

Change one variable at a time. Keep a known-good configuration to
compare against.

## 17.2 GPIO and LED failures

-   LED may be active-low.
-   Pin may be assigned to another peripheral.
-   GPIO clock may not be enabled.
-   Pin mode may not be output.
-   Board LED pin may differ from tutorial.
-   External load may exceed pin limits.
-   Code may never reach the toggle due to a fault or blocking call.

## 17.3 UART failures

-   Wrong baud rate or framing
-   Wrong TX/RX wiring
-   Missing common ground
-   Incorrect logic voltage
-   Wrong clock tree
-   UART pins configured incorrectly
-   Buffer lifetime issue in asynchronous transfer
-   Blocking transmit in an ISR or timing-sensitive loop

## 17.4 I²C failures

-   Wrong address format
-   Missing or unsuitable pull-ups
-   Incorrect voltage levels
-   Device not powered or held in reset
-   SDA/SCL swapped
-   Wrong timing configuration
-   Device needs a register-selection sequence or repeated START
-   Bus held low after a failed transaction

## 17.5 SPI failures

-   Wrong CPOL/CPHA
-   Wrong CS polarity or timing
-   Incorrect pin alternate function
-   Clock too fast for the peripheral
-   MISO disconnected or device not selected
-   Device requires dummy clocks or command bits
-   Incorrect byte order or address auto-increment behavior

## 17.6 ADC failures

-   Pin not in analog mode
-   Wrong channel/rank
-   Input exceeds permitted voltage
-   Reference voltage misunderstood
-   Sample time too short for source impedance
-   ADC clock/configuration invalid
-   Trigger source not actually routed to ADC
-   ADC conversion time longer than trigger interval
-   DMA width/alignment does not match expectations
-   Buffer is overwritten before processing

## 17.7 Timer failures

-   Wrong timer clock assumption
-   PSC/ARR off-by-one error
-   NVIC interrupt not enabled
-   IRQ handler not connected to HAL handler
-   Timer not started
-   Wrong timer instance checked in callback
-   Timer channel pin not configured as alternate function
-   Timer conflicts with HAL tick or another subsystem

## 17.8 DMA failures

-   Wrong request/channel/stream
-   Incorrect transfer direction
-   Incorrect data width
-   Memory increment disabled when needed
-   Buffer goes out of scope
-   DMA target memory is inaccessible
-   Cache not handled on cache-equipped cores
-   Callback does too much work
-   Application reads data before transfer completion
-   Circular DMA overwrites data before processing finishes

## 17.9 FreeRTOS failures

-   Task stack too small
-   Queue length too short
-   Task priority unsuitable
-   A task never blocks and starves others
-   Mutex deadlock
-   Priority inversion
-   Calling a non-ISR-safe API from an ISR
-   Interrupt priority incompatible with FreeRTOS syscall rules
-   Shared data accessed without synchronization
-   Blocking I/O inside a critical acquisition path

------------------------------------------------------------------------

# 18. Practical Lab Sequence

## Lab 1 --- GPIO and project workflow

**Goal:** understand the `.ioc` workflow and generated code.

1.  Create a project for the target board.
2.  Configure an LED pin as output.
3.  Generate code.
4.  Implement a blink.
5.  Change the interval and test.
6.  Locate the initialization code and user-code region.

**Questions:** Which code did CubeMX generate? Which statements define
application behavior? What changes if the LED is active-low?

## Lab 2 --- Timer interrupt

**Goal:** produce a periodic event.

1.  Configure a timer clock, prescaler and period.
2.  Enable the timer IRQ in NVIC.
3.  Start the timer in interrupt mode.
4.  Increment a counter or set a flag in the callback.
5.  Verify timing with a diagnostic pin if possible.

**Questions:** What is the actual timer input clock? Why should the
callback remain short?

## Lab 3 --- UART logging

**Goal:** send data to a PC.

1.  Configure UART pins and baud/framing.
2.  Connect a USB-to-UART adapter or supported virtual COM port.
3.  Transmit a known message.
4.  Verify the terminal settings.
5.  Compare blocking transmission with interrupt/DMA-based approaches.

**Questions:** What causes garbled text? How can blocking transmission
affect real-time behavior?

## Lab 4 --- ADC polling

**Goal:** convert an analog input.

1.  Configure the pin in analog mode.
2.  Configure ADC channel and sample time.
3.  Start conversion and read the result.
4.  Convert raw code to approximate voltage.
5.  Move a potentiometer and observe changes.

**Questions:** What is the reference voltage? What limits the input
range? Why is resolution not the same as accuracy?

## Lab 5 --- Timer + ADC + DMA

**Goal:** collect samples at a regular rate.

1.  Configure the timer event.
2.  Configure ADC external trigger.
3.  Configure DMA and a RAM buffer.
4.  Start acquisition.
5.  Inspect buffer contents and callbacks.
6.  Process completed blocks outside the callback.
7.  Increase sample rate gradually and watch for errors.

**Questions:** What is the per-channel sample rate? When can DMA
overwrite data? How would you detect a missed block?

## Lab 6 --- Sensor driver

**Goal:** separate sensor logic from application code.

1.  Read the sensor datasheet.
2.  Create `sensor_name.h` and `sensor_name.c`.
3.  Implement initialization and read functions.
4.  Return meaningful status codes.
5.  Document measurement units.
6.  Use the driver from application code without duplicating register
    transactions.

**Questions:** Could this driver be reused with a different I²C handle?
What assumptions remain family-specific?

## Lab 7 --- RTOS design exercise

**Goal:** decide whether an RTOS is helpful.

Sketch a sensor system with acquisition, processing and communication.
Identify:

-   Tasks and their responsibilities
-   Data passed between tasks
-   Queue/notification needs
-   Shared resources and mutex requirements
-   Timing and priority constraints
-   Failure behavior if a queue fills or a task cannot keep up

Do not add an RTOS merely to make the design look advanced. Explain the
benefit it provides.

------------------------------------------------------------------------

# 19. Instructor Preparation Checklist

-   [ ] Verify STM32CubeIDE/CubeMX and the correct MCU support package.
-   [ ] Prepare a known-good `.ioc` project and backup.
-   [ ] Test build, flash and debug before students arrive.
-   [ ] Confirm the board's exact MCU and clock source.
-   [ ] Prepare a GPIO blink demonstration.
-   [ ] Prepare a timer interrupt demonstration.
-   [ ] Prepare ADC polling code.
-   [ ] If hardware supports it, prepare timer-triggered ADC + DMA.
-   [ ] Prepare a UART terminal and known-good cable/adapter.
-   [ ] Choose one sensor and verify wiring, datasheet, and driver.
-   [ ] Prepare a driver header/source example.
-   [ ] Prepare a FreeRTOS task/queue example or architecture diagram.
-   [ ] Save screenshots and code backups in case live hardware fails.
-   [ ] Map every lab to the concept it demonstrates.
-   [ ] Decide which advanced details are optional if time runs short.

------------------------------------------------------------------------

# 20. Knowledge Check

## Questions

1.  What does the `.ioc` file store?
2.  Why can regenerating code overwrite application changes?
3.  What is the difference between CubeMX and CubeIDE?
4.  What is the difference between HAL, LL, and register-level code?
5.  What are the different roles of timer, ADC, DMA, interrupt, and
    application code?
6.  When is polling a better choice than DMA?
7.  Why can a timer clock differ from SYSCLK?
8.  What do PSC and ARR represent?
9.  Why does I²C usually need pull-up resistors?
10. What do CPOL and CPHA control in SPI?
11. Why can a UART terminal show garbled characters?
12. What is the difference between ADC resolution and accuracy?
13. What is sampling jitter?
14. Why can a circular DMA buffer lose data even when DMA is functioning
    correctly?
15. What makes a sensor driver reusable?
16. When might a superloop be preferable to FreeRTOS?
17. What is the difference between a queue, semaphore, and mutex?
18. Why must an ISR remain short?
19. Why is `volatile` not a complete synchronization strategy?
20. How would you debug a peripheral that is not responding?

## Answer key

1.  CubeMX configuration such as target, pinout, clocks, peripheral
    settings, and code-generation options.
2.  Changes outside protected user-code sections may be replaced by
    generated output.
3.  CubeMX configures hardware and generates code; CubeIDE edits,
    builds, flashes, and debugs the project.
4.  HAL is higher-level, LL is more direct, and register-level code
    accesses hardware registers directly.
5.  Timer schedules/triggers; ADC converts; DMA moves data; interrupt
    notifies CPU; application interprets/processes.
6.  For small, infrequent operations where simplicity is more valuable
    than CPU efficiency.
7.  Bus prescalers and peripheral clock selection affect the clock
    delivered to a timer or peripheral.
8.  Prescaler and auto-reload register values; common formulas use each
    value plus one.
9.  I²C lines are typically open-drain and need pull-ups to establish
    the high level.
10. SPI clock polarity and phase, which define clock idle level and
    sampling/transition timing.
11. Baud/framing mismatch, wrong clock, wiring, or voltage/interface
    problems.
12. Resolution is the number of codes; accuracy is closeness to the true
    value.
13. Variation in the time between samples or relative to the intended
    sampling schedule.
14. The CPU may not process a completed region before DMA wraps and
    overwrites it.
15. Clear API, documented units, explicit dependencies, error handling,
    and limited coupling to application code.
16. When the system is small, timing is simple, and functions are
    short/non-blocking.
17. Queue transfers messages/data; semaphore signals/coordinates; mutex
    provides mutual exclusion for a shared resource.
18. Long ISRs delay other work and can cause latency, missed deadlines,
    or priority problems.
19. `volatile` does not by itself make compound operations atomic or
    coordinate multiple contexts safely.
20. Check hardware, exact part, pins, clocks, configuration, IRQ/DMA,
    application sequence, data interpretation, and timing in that order.

------------------------------------------------------------------------

# 21. Quick-Reference Formulas and Decision Tables

## Timer formulas

For ordinary up-counting:

\[ f\_{`\text{counter}`{=tex}} = `\frac{f_{\text{TIM}}}{PSC+1}`{=tex} \]

\[ f\_{`\text{update}`{=tex}} =
`\frac{f_{\text{TIM}}}{(PSC+1)(ARR+1)}`{=tex} \]

Always verify the actual timer clock and counter mode.

## ADC formulas

For an ideal N-bit ADC:

\[ `\text{Code}`{=tex}
`\approx `{=tex}`\frac{V_{\text{in}}}{V_{\text{ref}}}`{=tex}(2\^N-1) \]

\[ V\_{`\text{in}`{=tex}}
`\approx `{=tex}`\frac{\text{Code}}{2^N-1}`{=tex}V\_{`\text{ref}`{=tex}}
\]

These are ideal approximations, not a substitute for calibration or
datasheet limits.

## Data rate

\[ R\_{`\text{raw}`{=tex}} = f_s `\times `{=tex}C `\times `{=tex}B \]

where (f_s) is samples per second, (C) is channel count, and (B) is
bytes per sample.

## Buffer duration

\[ T\_{`\text{buffer}`{=tex}} = `\frac{N}{f_s}`{=tex} \]

Clarify whether N counts total conversion results or samples per
channel.

## Fast decision table

  Requirement                                           Starting point
  ----------------------------------------------------- --------------------------------------
  Simple digital output                                 GPIO
  Detect button edge                                    EXTI interrupt + debounce
  Stable periodic event                                 Hardware timer
  PWM waveform                                          Timer PWM
  Occasional analog read                                ADC polling or interrupt
  Fixed-rate analog sampling                            Timer-triggered ADC
  Continuous ADC blocks                                 Timer + ADC + DMA
  PC debug link                                         UART
  Several compatible low-rate sensors                   I²C
  High-throughput peripheral link                       SPI or dedicated interface
  Reusable sensor support                               Driver API + separate implementation
  Small predictable application                         Superloop
  Multiple independent activities and synchronization   Consider RTOS

------------------------------------------------------------------------

# Closing Principle

Reliable embedded systems are built by choosing the correct hardware
feature for the requirement, configuring it correctly, understanding the
complete data path, handling timing and errors, and keeping application
code modular.

The goal is not merely to remember which buttons to click in CubeMX.
Students should be able to explain **why a design works, how to verify
it, and how to diagnose it when it fails**.
