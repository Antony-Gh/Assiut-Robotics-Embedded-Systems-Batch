# 2. Part 1 — STM32 Development Workflow

## 2.1 STM32 Internal Architecture and the Bus Matrix

A modern STM32 does not route every access through one bottlenecked bus. Bus **masters** (the Cortex-M core through its I-Code, D-Code and System buses, and the DMA controllers) are connected through a **multi-layer AHB bus matrix** to bus **slaves** (Flash, SRAM banks, AHB peripherals and the APB bridges). Because the matrix is a crossbar, the CPU can fetch instructions from Flash while a DMA controller moves ADC samples into SRAM, without the two stalling each other, unless both target the same slave in the same cycle.

**Table 2.1 — Buses on an STM32F1/F4-class device**

| Bus | Hosts (examples) | Typical limit | Notes |
| --- | --- | --- | --- |
| AHB (HCLK) | Core, DMA, SRAM, Flash interface, GPIO on F4, RCC, CRC | F103: 72 MHz | Fast bus; newer families also place ADCs and GPIO here |
| APB1 (PCLK1, low speed) | TIM2–TIM7, USART2/3/UART4/5, I2C1/2, SPI2/3, CAN, DAC, PWR, IWDG, WWDG | F103: max 36 MHz | On F103 also the bus whose prescaler triggers the timer ×2 rule |
| APB2 (PCLK2, high speed) | TIM1/TIM8, USART1, SPI1, ADC1/2/3, EXTI/AFIO or SYSCFG, GPIO on F1 | F103: max 72 MHz | Faster peripherals live here |

> [!NOTE]
> **Why this matters to firmware.** The same peripheral type can live on a different bus in a different family (for example GPIO is on APB2 in F1 but on AHB1 in F4). Always read the clock-enable macro that CubeMX generates (`__HAL_RCC_GPIOA_CLK_ENABLE()`) rather than assuming a bus.

## 2.2 The Toolchain Ecosystem

Professional STM32 development uses a set of cooperating tools, each with a strict responsibility:

- **STM32CubeMX 6.18.1** is a graphical configurator and initialization-code generator. It handles MCU or board selection, pinout and conflict resolution, the clock tree, peripheral parameters, NVIC and DMA routing and middleware (FreeRTOS, USB, FatFS, LwIP). It stores its state in the human-readable **`.ioc`** file.
- **STM32CubeIDE 2.2.0** is the Eclipse/CDT-based IDE that embeds the CubeMX configurator and adds the C/C++ editor, build manager and SWD/JTAG debugger front end (ST-LINK, J-Link) with register (SFR) views, live expressions, SWV/ITM trace and a fault analyzer.
- **GNU Arm toolchain** (`arm-none-eabi-gcc`, `as`, `ld`, `objcopy`, `size`) turns C and assembly into `.elf`, `.bin` and `.hex` images placed according to the linker script.
- **STM32CubeProgrammer** (and the ST-LINK GDB server inside CubeIDE) programs Flash and talks to the debug port.
- **Firmware packages** (STM32Cube_FW_xx) supply CMSIS headers and the HAL/LL drivers for each family.

**Table 2.2 — Tool and artifact responsibilities versus misconceptions**

| Tool / artifact | Actual responsibility | Misconception (what it does *not* do) |
| --- | --- | --- |
| CubeMX | Pinout, alternate functions, clock tree, peripheral init, middleware, code generation | Does not write your application state machine, frame packets or handle faults |
| CubeIDE | Editing, GCC build, link, SWD flashing, debugging | A warning-free build proves nothing about wiring, clocks or timing |
| `.ioc` file | Text blueprint of MCU, pins, clocks, settings | It is not firmware and is never flashed; commit it to Git with the sources |
| CMSIS-Core / device header | ARM-standard core registers (SCB, NVIC, SysTick) and device register structs | CMSIS is the register foundation, not the ST HAL |
| HAL / LL drivers | ST C driver abstractions for a specific family | APIs and capabilities are not identical across F1, F4, G4, H7 |
| Startup file (`startup_*.s`) | Reset handler: set stack pointer, copy `.data`, zero `.bss`, call `SystemInit()` then `main()` | If the vector table or `SystemInit()` faults, execution never reaches `main()` |
| Linker script (`*_FLASH.ld`) | Flash/SRAM addresses, sizes and section placement | Linking for 512 KB but flashing a 128 KB part causes faults |
| SWD probe | Halts the CPU, programs Flash, reads/writes memory and peripherals | A breakpoint freezes the CPU while sensors and timers keep moving |

## 2.3 Installation and Setup

The following sequence gives a reproducible environment. Menu names are those of the 1.x/2.x CubeIDE generation; confirm against the release notes of your exact version (Appendix B).

1. **Prepare the PC.** Use a path without spaces or non-ASCII characters for the workspace (for example `C:\stm32_ws`). Keep the path short; very long generated paths can break the build on Windows.
2. **Install STM32CubeIDE 2.2.0.** The installer bundles the GNU Arm toolchain, the ST-LINK GDB server and CubeMX integration. Accept the ST-LINK driver installation.
3. **Install STM32CubeMX 6.18.1 (optional standalone).** CubeIDE already contains the configurator; a standalone copy is useful for reviewing `.ioc` files without opening an IDE workspace.
4. **Download the family firmware package.** In CubeMX: *Help → Manage embedded software packages*, then install the package for your family (for example STM32CubeF1). CubeIDE offers to do this the first time you create a project.
5. **Update ST-LINK firmware** (CubeProgrammer or the ST-LINK upgrade tool) if the debugger reports a version mismatch.
6. **Verify the whole chain** with a blink project (Lab 1 in Chapter 8): create → generate → build → flash → LED toggles after power-cycling *without* the debugger attached.

> [!TIP]
> **Keep a known-good project.** Save a working “blink + UART hello” project before experimenting with clocks or multiple peripherals. When something stops working, diff against it.

## 2.4 Creating a Project

![Figure 2.1 — CubeMX project lifecycle and software abstraction stack](diagrams/fig2_workflow_and_hal.png)

*Figure 2.1 — (A) `.ioc`-to-firmware lifecycle; (B) software abstraction stack from application to silicon.*

Follow this disciplined sequence in CubeIDE 2.2.0 / CubeMX 6.18.1:

1. **Verify the silicon and schematic.** Read the chip marking (`STM32F103C8T6` vs. `STM32F401RCT6`), identify the package (LQFP48/64, UFQFPN32) and check whether an external crystal (HSE) is fitted and at which frequency. Smaller packages do not bond out every pad.
2. **Select the target.** `File → New → STM32 Project`; choose the exact commercial part number (or the board, which preloads LEDs, buttons and clocks).
3. **Set SYS first.** `System Core → SYS → Debug = Serial Wire`. Leaving *No Debug* disables PA13/PA14 (SWDIO/SWCLK) after reset and can lock you out of the chip.
4. **Choose the HAL timebase.** With bare-metal code SysTick is fine. If you will use FreeRTOS, change *Timebase Source* to a spare timer (TIM6/TIM7 or a basic timer) so the RTOS owns SysTick while `HAL_Delay()` and HAL timeouts keep working.
5. **Configure RCC.** Enable HSE as *Crystal/Ceramic Resonator* (or *BYPASS Clock Source* for an external oscillator) if the board has one.
6. **Assign pins and labels.** Give pins meaningful User Labels (`STATUS_LED`, `IMU_CS`, `FAULT_IN`) so generated macros read like intent.
7. **Enable peripherals one at a time.** Set parameters, NVIC interrupts/priorities and DMA requests, and resolve conflicts deliberately.
8. **Set Project Manager options.** Under *Code Generator* enable *Generate peripheral initialization as a pair of .c/.h files per peripheral* and *Keep User Code when re-generating*; set a toolchain/IDE of *STM32CubeIDE*.
9. **Generate, inspect and commit.** Save (`Ctrl+S`) to generate. Review the Git diff: regeneration may change more than the setting you touched.

## 2.5 The `.ioc` File and the CubeMX Workspace

The `.ioc` file records everything CubeMX needs to regenerate the initialization code: target MCU/board, pin assignments, clock settings, enabled peripheral instances and parameters, interrupt and DMA configuration, middleware and code-generation options. **It is the configuration source, not the firmware.**

**Table 2.3 — CubeMX tabs and what they control**

| Tab / section | Purpose | Typical decisions |
| --- | --- | --- |
| Pinout & Configuration → Categories | Enable peripherals and set parameters | System Core (GPIO, DMA, NVIC, RCC, SYS, IWDG/WWDG), Analog (ADC, DAC), Timers, Connectivity (UART, SPI, I2C, CAN, USB), Middleware (FreeRTOS, FatFS) |
| Pinout view | Assign pins, alternate functions, labels | Resolve conflicts; confirm the pin exists on your package |
| Clock Configuration | Source selection, PLL, prescalers, bus and peripheral clocks | HSE vs. HSI, PLL multiplier, HCLK/PCLK limits, timer clock box |
| Project Manager → Project | Name, location, toolchain, stack/heap size | CubeIDE toolchain; minimum heap/stack sizes |
| Project Manager → Code Generator | File layout, library copying, user-code retention | One `.c/.h` per peripheral; keep user code |
| Project Manager → Advanced Settings | Driver choice per peripheral (HAL or LL), init order | Select HAL by default; LL where needed |

**Typical workflow:** create project → pins → clocks → peripherals → parameters → NVIC/DMA/middleware → save `.ioc` → generate → review warnings → add application logic in user-code regions or separate files → build, flash, test → commit `.ioc` and sources together.

**Code-generation cautions**

- Keep application logic outside generated regions that are not protected by `USER CODE` markers.
- Prefer separate application and driver files for anything non-trivial.
- After changing the `.ioc`, read the diff rather than trusting regeneration.
- Resolve pin conflicts intentionally instead of selecting random pins.
- If a generated function is missing, check that the peripheral is enabled, generation finished, and the right configuration is active.

## 2.6 GPIO Architecture, Alternate Functions and Electrical Behavior

**Table 2.4 — GPIO operating modes**

| Mode | Internal configuration | Electrical behavior | Typical use |
| --- | --- | --- | --- |
| Input floating | Output driver off; Schmitt trigger on; pulls off | High impedance; floats if undriven | Externally driven digital lines |
| Input pull-up / pull-down | Output off; Schmitt on; ~40 kΩ pull on | Weak bias to VDD or GND | Push-buttons, fault lines |
| Output push-pull | PMOS and NMOS active | Actively drives 1 and 0 | LEDs, SPI chip select, enable pins |
| Output open-drain | PMOS off, NMOS only | Pulls low or releases (needs pull-up) | Wired-AND lines, level shifting, bit-banged buses |
| Alternate function push-pull | Peripheral controls push-pull driver | Peripheral drives both edges | UART TX, SPI SCK/MOSI, PWM |
| Alternate function open-drain | Peripheral controls open-drain driver | Peripheral pulls low or releases | I²C SDA/SCL, 1-Wire |
| Analog | Output off; Schmitt off; pulls off; analog switch on | No digital switching noise | ADC/DAC/comparator pins |

The **output speed** setting is a slew-rate control, not a clock. `HIGH`/`VERY_HIGH` increases edge rates and therefore ringing, overshoot and EMI; use the slowest setting that meets your rise-time budget.

> [!CAUTION]
> **Electrical danger: 5 V tolerance and current limits.**
>
> 1. Never assume a pin is 5 V tolerant. Look for the **FT** marking in the datasheet pinout table. Even on FT pins, tolerance applies only to digital input mode with VDD powered. **Never** apply more than VDD to an analog (ADC) pin: injection current corrupts other channels or latches up the silicon.
> 2. A GPIO pin is rated for roughly 8–25 mA depending on family and mode, with strict total package current limits. Never drive motors, solenoids or relay coils directly. Use a MOSFET/BJT driver with a flyback diode.
> 3. Check board schematics: an on-board LED may be active-low (Blue Pill PC13 lights when the pin is **low**) or shared with another function.

## 2.7 Clock Tree, PLL and the APB Timer ×2 Rule

A peripheral can be configured perfectly in its own panel and still misbehave if the clock tree is wrong. Clock settings affect CPU speed, bus frequencies, UART baud accuracy, timer periods and PWM frequencies, ADC clock and conversion time, USB (which needs an accurate 48 MHz) and Flash wait states.

**Table 2.5 — Clock sources and nodes**

| Name | Role | Notes |
| --- | --- | --- |
| HSI | Internal high-speed RC | Always present; accuracy of a few percent unless trimmed |
| HSE | External crystal, resonator or bypass | ppm-level accuracy with the right load capacitors; verify the board really has one |
| LSI | Internal low-speed RC | Watchdog, sometimes RTC; very inaccurate |
| LSE | 32.768 kHz crystal | RTC timekeeping |
| PLL | Multiplies/divides HSI or HSE | Keep the VCO within the reference-manual range |
| SYSCLK → HCLK | System and AHB clock | Feeds core, DMA, memories |
| PCLK1 / PCLK2 | APB1 / APB2 clocks | Peripheral clocks and timer-clock source |
| Kernel clocks | Per-peripheral clock mux (newer families) | A UART may run from a different PLL output than the CPU |

**Worked example — STM32F103C8T6 at 72 MHz.** HSE = 8 MHz, PLL source HSE, PLLMUL = ×9 → SYSCLK = 72 MHz. AHB prescaler /1 → HCLK = 72 MHz. APB1 prescaler /2 → PCLK1 = 36 MHz (the maximum for APB1). APB2 prescaler /1 → PCLK2 = 72 MHz. ADC prescaler /6 → ADCCLK = 12 MHz (limit 14 MHz). Flash latency = 2 wait states for 48–72 MHz.

> [!WARNING]
> **The APB timer ×2 trap.** On classic F1/F2/F3/F4/F7/L4 trees the timer kernel clock is derived as follows: if the APB prescaler is 1, `f_TIM = PCLK`; if the prescaler is greater than 1, **`f_TIM = 2 × PCLK`**. With APB1 /2 on an F103 at 72 MHz, `PCLK1 = 36 MHz` but TIM2/3/4 are clocked at **72 MHz**. Always read the “To APB1/APB2 timer clocks” box in CubeMX. Newer families (G0, G4, H7, U5) may use different timer-clock schemes, so do not apply F4 folklore blindly. A period that is off by exactly a factor of two is the signature of this rule; check the clock tree before changing values at random.

**Clock-configuration workflow.** Open the `.ioc`, go to Clock Configuration, identify the board’s *real* clock source and frequency (do not assume a crystal exists because the MCU supports HSE), choose the system clock source, configure the PLL, check every bus and peripheral clock against its limit, check Flash latency, resolve CubeMX warnings, save/regenerate, read `SystemClock_Config()`, then verify with a known test (UART text, a toggling pin measured with a scope). Never copy clock settings from a tutorial written for a different board.

## 2.8 Code Generation, Project Structure and USER CODE Preservation

A typical generated HAL project contains:

```text
Project/
├── Core/
│   ├── Inc/      main.h, stm32f1xx_it.h, stm32f1xx_hal_conf.h, <peripheral>.h
│   └── Src/      main.c, stm32f1xx_it.c, stm32f1xx_hal_msp.c, <peripheral>.c, syscalls.c
├── Drivers/
│   ├── CMSIS/                    core + device headers
│   └── STM32F1xx_HAL_Driver/     HAL/LL sources
├── Middlewares/                  only if enabled (FreeRTOS, USB, FatFS)
├── startup_stm32f103xb.s         vector table + reset handler
├── STM32F103C8TX_FLASH.ld        linker script
└── Project.ioc
```

**Generated functions and their jobs**

- `HAL_Init()` configures Flash prefetch/caches (family dependent), sets the NVIC priority grouping, starts the 1 ms time base (SysTick by default) and calls `HAL_MspInit()`.
- `SystemClock_Config()` programs oscillators, PLL, prescalers and Flash latency from the Clock Configuration tab.
- `MX_GPIO_Init()`, `MX_USARTx_UART_Init()`, `MX_ADCx_Init()`, `MX_TIMx_Init()` and so on fill the peripheral handle structures and call `HAL_xxx_Init()`.
- `HAL_xxx_MspInit()` (in `*_hal_msp.c`) does the board-level part: peripheral clock enable, GPIO alternate function setup, NVIC enable and DMA linking.
- `stm32f1xx_it.c` holds the interrupt handlers, each of which calls the matching `HAL_xxx_IRQHandler()`.

**Listing 2.1 — Generated clock setup for F103 at 72 MHz (HSE 8 MHz, PLL ×9)**

```c
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                     RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   /* PCLK1 = 36 MHz */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;   /* PCLK2 = 72 MHz */
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}
```

**Listing 2.2 — `main.c` skeleton showing initialization order and USER CODE regions**

```c
/* USER CODE BEGIN Includes */
#include "app.h"
#include "mpu6050.h"
/* USER CODE END Includes */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();            /* MUST come before peripherals that use DMA */
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  App_Init();
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN WHILE */
    App_Update();
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
  }
}
```

> [!CAUTION]
> **Common mistake: editing outside USER CODE.** Anything outside a `USER CODE BEGIN … END` pair is regenerated and erased. CubeMX offers protected regions for includes, private types/defines/macros/variables, function prototypes, `main()` stages (0–4, `WHILE`), the clock/MSP functions and each interrupt file. Put your real logic in `app.c`, `sensor.c` and similar files and call them from a short USER CODE block. Also place `MX_DMA_Init()` **before** DMA-using peripherals; if it is after, the DMA handle link breaks silently (this ordering is a CubeMX-generated default you should not reorder by hand).

## 2.9 HAL, LL and Direct Register Programming

### 2.9.1 What each layer is

- **Register-level (CMSIS) programming** reads and writes the memory-mapped peripheral registers through the structures in the device header (`GPIOC->BSRR`, `USART2->DR`). It is the truth: every other layer ends here.
- **LL (Low-Layer)** is a set of thin `static inline` helpers named after the register fields (`LL_GPIO_ResetOutputPin()`), with no state tracking, timeouts or locks.
- **HAL (Hardware Abstraction Layer)** adds handle structures (`UART_HandleTypeDef`), a state machine (`HAL_UART_STATE_*`), locking, timeouts, status return codes and callbacks. It is what CubeMX targets by default.

### 2.9.2 One operation, three layers

The same operation, driving PC13 low, written at each level (F1; PC13 on a Blue Pill LED is active-low):

**Listing 2.3 — Setting PC13 low with HAL, LL and registers**

```c
/* 1. HAL */
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

/* 2. LL */
LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13);

/* 3. Register: BSRR upper 16 bits reset the pin, lower 16 bits set it */
GPIOC->BSRR = (uint32_t)GPIO_PIN_13 << 16U;
```

`HAL_GPIO_WritePin()` is essentially the third line plus an argument assertion. That is the general pattern: **HAL function → (checks, state, timeouts) → register writes → silicon.** For complex peripherals the HAL value is far larger, because it sequences start/stop/ACK handling (I²C), interrupt and DMA bookkeeping, and error flags.

**Listing 2.4 — Bare-register PC13 setup on F1 (for understanding only)**

```c
RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;          /* clock to GPIOC (APB2 on F1)         */
GPIOC->CRH &= ~(0xFUL << 20);               /* clear CNF13[1:0] and MODE13[1:0]     */
GPIOC->CRH |=  (0x2UL << 20);               /* MODE13 = 10 (2 MHz), CNF13 = 00 PP   */
GPIOC->BSRR = (uint32_t)GPIO_PIN_13 << 16U; /* drive low (LED on)                   */
```

This assumes F1 register layout (`CRL/CRH`). On F4 the same configuration uses `RCC->AHB1ENR` and `GPIOC->MODER`. This is exactly why register code is non-portable.

### 2.9.3 Comparison and selection

**Table 2.6 — HAL vs. LL vs. direct registers**

| Dimension | HAL | LL | Direct registers |
| --- | --- | --- | --- |
| Abstraction | High: handles, state machines, callbacks, timeouts | Medium-low: inline helpers with ST field names | Lowest: raw memory-mapped structs |
| Development speed | Fastest; CubeMX generates init | Moderate; you manage state | Slowest; read the reference manual |
| Portability | High across families (minor changes) | Moderate; tied to the peripheral IP version | Low; names and semantics change (`SR` vs. `ISR`, `DR` vs. `TDR/RDR`) |
| Code size / overhead | Larger; tens of cycles per call is typical | Near zero | Minimal |
| Safety | Timeouts (`HAL_TIMEOUT`), state (`HAL_BUSY`) | No automatic timeouts or locks | High risk: reserved bits, write-1-to-clear traps, read-modify-write races |
| Best use | Most application firmware; I²C, USB, CAN, DMA state machines | Fast ISRs (for example a 50 kHz control loop) | Deep study, cycle-critical loops, features HAL does not expose |

**Recommendation.** Start with CubeMX and HAL. Read the HAL source of the functions you call. Learn the registers from the reference manual. Measure. Optimize only after you have identified a real bottleneck. Do not teach that register code is automatically faster or that HAL is automatically slow: choose the lowest level that solves the problem safely and maintainably.

## 2.10 Debugging Workflow in CubeIDE

- **Breakpoints and stepping** on the ST-LINK GDB server; **Live Expressions** to watch variables while the program runs; the **SFR view** to read and write peripheral registers (invaluable for confirming what HAL really configured).
- **SWV/ITM** can stream `printf` output and data traces over the SWO pin without using a UART, where the board and probe wire SWO.
- **Halting freezes only the CPU.** Timers, ADC and external sensors keep running, so after a breakpoint the data stream is stale. Many parts provide DBGMCU freeze bits to stop timers and watchdogs while halted; a watchdog that keeps running will reset the MCU under the debugger.
- Print the **reset cause** and the HAL return codes of every call you care about; do not rely on silence.
- When execution lands in `HardFault_Handler`, use the analysis method in Section 8.2.

> [!NOTE]
> **Key takeaway (Part 1).** CubeMX configures hardware and generates initialization; CubeIDE edits, builds, flashes and debugs; the `.ioc` is the configuration source and belongs in version control; regeneration only preserves code inside USER CODE regions; and clocks are the first thing to verify when any peripheral misbehaves.
