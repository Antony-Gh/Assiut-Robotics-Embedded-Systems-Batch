# 5. Topic 4 — Linker Scripts, Memory Sections, and Startup Internals

To the majority of novice embedded developers, compiling code is an opaque black box: they click the "Build" button in an IDE, and somehow their C source code becomes binary machine instructions executing on silicon.

However, when a real-world firmware project fails to link—yielding cryptic errors such as `region 'RAM' overflowed by 2048 bytes`, `undefined reference to '_sbrk'`, or unexpected HardFaults during variable initialization—engineers who lack an understanding of the compilation pipeline, linker scripts, and startup assembly are left helpless.

Embedded C programming differs fundamentally from desktop software. On an STM32, **there is no underlying operating system (like Linux or Windows) to allocate memory, load an executable into RAM, zero variables, or jump to `main()`**. 

The firmware must configure its own execution environment from raw silicon registers.

This chapter demystifies the complete toolchain build process, performs a line-by-line architectural analysis of the ARM Cortex-M startup assembly file (`startup_stm32f103xb.s`), dissects the GNU linker script (`STM32F103C8TX_FLASH.ld`), visualizes physical Flash and SRAM memory mapping, and demonstrates how to audit `.map` files to diagnose memory exhaustion.

![Figure 4.1 — Embedded compilation pipeline, startup sequence, and physical memory layout](diagrams_adv/fig4_build_and_memory_layout.png)
*Figure 4.1 — (A) The GNU ARM compilation, assembly, linking, and flashing pipeline; (B) Physical memory map contrasting Flash storage (.isr_vector, .text, .rodata, .data initializers) with runtime SRAM (.data, .bss, Heap, Stack).*

---

## 5.1 The Complete Journey: From C Source to Executable Silicon

Transforming human-readable C source code into physical electrical voltages executed by an ARM processor requires a deterministic 6-stage toolchain pipeline orchestrated by the **GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`)**.

```text
 [*.c Source Files] ────────► Preprocessor (gcc -E) ────────► [*.i Expanded Files]
                                                                    │
                                                                    ▼
 [*.s Assembly Files] ◄────── Compiler (gcc -S) ◄───────────────────┘
          │
          ▼
   Assembler (as) ──────────► [*.o Relocatable Object Files]
                                     │
                                     ▼
 [Linker Script (*.ld)] ────► Linker (ld) ◄────────────────── [Static Libs (*.a)]
                                     │
                                     ▼
                              [Firmware.elf] (Executable and Linkable Format)
                                     │
                     ┌───────────────┴───────────────┐
                     ▼                               ▼
       objcopy -O binary Firmware.bin   objcopy -O ihex Firmware.hex
```

### The 6 Build Stages Explained
1. **Preprocessing (`arm-none-eabi-gcc -E`)**: Resolves all preprocessor directives: expands `#include` header files, replaces `#define` macros, evaluates conditional compilation blocks (`#ifdef`), and strips comments, outputting intermediate preprocessed files (`.i`).
2. **Compilation (`arm-none-eabi-gcc -S`)**: Parses C syntax, generates Abstract Syntax Trees (AST), performs compiler optimizations (`-O0`, `-O2`, `-Os`), and translates C logic into ARM **Thumb-2 assembly language instructions** (`.s`).
3. **Assembly (`arm-none-eabi-as`)**: Converts textual assembly mnemonics into raw binary machine opcodes, generating **Relocatable Object Files (`.o`)**. In an object file, internal function calls and variable addresses are not yet assigned fixed physical memory locations; they are marked with placeholder relocation entries.
4. **Linking (`arm-none-eabi-ld`)**: The critical integration phase. The linker consumes all individual `.o` object files, static libraries (`libc.a`, `libgcc.a`), and the **Linker Script (`*.ld`)**. The linker assigns absolute physical memory addresses (in Flash and SRAM) to every function, constant, and global variable, and binds all cross-file symbol references into a unified **Executable and Linkable Format (`.elf`)** file.
5. **Binary Extraction (`arm-none-eabi-objcopy`)**: While `.elf` files contain extensive debugging symbol tables, section headers, and source-line metadata used by GDB, the STM32 hardware Flash controller only accepts raw contiguous binary bytes. `objcopy` extracts raw memory bytes into a **`.bin`** file or an **Intel `.hex`** record file.
6. **Flashing & Execution**: The ST-LINK programmer writes the `.bin` image into on-chip Flash starting at base address `0x08000000`. When the hardware reset line (`NRST`) is released, the ARM core fetches the initial Stack Pointer from `0x08000000` and the entry address of `Reset_Handler` from `0x08000004`.

---

## 5.2 Deep-Dive: What Happens Before `main()`? The Startup Code

When an STM32 boots up from cold power-on, **`main()` is NOT the first function executed**.
The microcontroller first executes a low-level assembly file generated by ST: **`startup_stm32f103xb.s`** (or `startup_stm32f405xx.s`).

### 5.2.1 The Interrupt Vector Table (`.isr_vector`)
At the very beginning of the startup assembly file, the programmer declares the **Interrupt Vector Table**:

```assembly
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word _estack                    /* 0x0000: Initial Main Stack Pointer (MSP) */
  .word Reset_Handler              /* 0x0004: Reset Handler Entry Point */
  .word NMI_Handler                /* 0x0008: Non-Maskable Interrupt */
  .word HardFault_Handler          /* 0x000C: HardFault Exception */
  .word MemManage_Handler          /* 0x0010: MPU Memory Fault */
  .word BusFault_Handler           /* 0x0014: Bus Fault */
  .word UsageFault_Handler         /* 0x0018: Usage Fault */
  .word 0                          /* Reserved */
  .word 0
  .word 0
  .word 0
  .word SVC_Handler                /* 0x002C: Supervisor Call */
  .word DebugMon_Handler           /* 0x0030: Debug Monitor */
  .word 0
  .word PendSV_Handler             /* 0x0038: PendSV (RTOS Context Switch) */
  .word SysTick_Handler            /* 0x003C: SysTick 1 ms Tick */

  /* External Peripheral Interrupt Vectors (IRQ0, IRQ1, ...) */
  .word WWDG_IRQHandler            /* Window Watchdog */
  .word PVD_IRQHandler             /* PVD through EXTI Line */
  .word TAMPER_IRQHandler
  .word RTC_IRQHandler
  .word FLASH_IRQHandler
  .word RCC_IRQHandler
  .word EXTI0_IRQHandler           /* External Interrupt 0 */
  ...
```

#### Why `_estack` is the First Word
The ARM Cortex-M hardware architecture mandates that address `0x00000000` (which is aliased to Flash base `0x08000000` when `BOOT0 = 0`) must contain the **initial value of the Main Stack Pointer (MSP)**.
Before the processor can execute even a single assembly instruction, it must have a valid stack pointer in place so that exceptions can push register frames.

---

### 5.2.2 Dissecting `Reset_Handler`
Immediately upon release of reset, the CPU core loads register `R15` (Program Counter) with the address stored at `0x08000004`—which points directly to **`Reset_Handler`**.

**Listing 5.1 — The core initialization sequence inside `startup_stm32f103xb.s`**

```assembly
  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack         /* 1. Ensure stack pointer is initialized to end of RAM */

/* 2. Copy the data segment initializers from Flash to SRAM */
  ldr   r0, =_sdata          /* R0 = Start of RAM .data segment */
  ldr   r1, =_edata          /* R1 = End of RAM .data segment */
  ldr   r2, =_sidata         /* R2 = Start of .data values stored in Flash */
  movs  r3, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r4, [r2, r3]         /* Load 32-bit word from Flash */
  str   r4, [r0, r3]         /* Store 32-bit word into SRAM */
  adds  r3, r3, #4           /* Advance index by 4 bytes */

LoopCopyDataInit:
  adds  r4, r0, r3
  cmp   r4, r1               /* Have we reached _edata? */
  bcc   CopyDataInit         /* Loop until all initialized globals are copied */

/* 3. Zero-initialize the .bss segment in SRAM */
  ldr   r2, =_sbss           /* R2 = Start of RAM .bss segment */
  ldr   r4, =_ebss           /* R4 = End of RAM .bss segment */
  movs  r3, #0
  b     LoopFillZerobss

FillZerobss:
  str   r3, [r2]             /* Write 0x00000000 into SRAM */
  adds  r2, r2, #4           /* Advance pointer by 4 bytes */

LoopFillZerobss:
  cmp   r2, r4               /* Have we reached _ebss? */
  bcc   FillZerobss          /* Loop until entire .bss is zeroed */

/* 4. Call CMSIS system initialization (Clock tree & FPU) */
  bl    SystemInit

/* 5. Call standard C library runtime initialization (Global C++ constructors) */
  bl    __libc_init_array

/* 6. Transfer control to your C application! */
  bl    main

/* 7. Safety Catch: If main() ever returns, trap CPU in an infinite loop */
LoopForever:
  b     LoopForever
```

### The Three Critical Tasks of `Reset_Handler`
1. **Copying `.data` from Flash to SRAM**: If you write `int motor_speed = 100;` in C, the initial value `100` is compiled into Flash ROM. When the MCU reboots, SRAM contains random garbage. `Reset_Handler` copies `100` from Flash (`_sidata`) into its designated SRAM address (`_sdata`).
2. **Zero-Initializing `.bss`**: If you declare `int sensor_buffer[256];` without an initializer, the C language standard guarantees that uninitialized variables default to zero. `Reset_Handler` loops through SRAM from `_sbss` to `_ebss`, writing `0x00000000` to every word.
3. **Hardware FPU & System Setup (`SystemInit`)**: On Cortex-M4 (STM32F405), `SystemInit()` enables the hardware Floating-Point Unit by writing `0x00F00000` into `SCB->CPACR` (Coprocessor Access Control Register). Without this, the first floating-point instruction would trigger an immediate `NOCP` Usage Fault!

---

## 5.3 Demystifying the Linker Script (`*.ld`)

The Linker Script is the master architectural blueprint that instructs the GNU linker where to place sections in physical memory.

### 5.3.1 Dissecting `STM32F103C8TX_FLASH.ld`

**Listing 5.2 — Annotated GNU Linker Script for STM32F103C8T6**

```ld
/* Master Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack (End of RAM) */
_estack = ORIGIN(RAM) + LENGTH(RAM); /* 0x20000000 + 20K = 0x20005000 */

_Min_Heap_Size  = 0x200; /* Minimum requested heap (512 bytes) */
_Min_Stack_Size = 0x400; /* Minimum requested stack (1024 bytes) */

/* Physical Silicon Memory Boundaries */
MEMORY
{
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K   /* 20 KB On-Chip SRAM */
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K   /* 64 KB On-Chip Flash */
}

SECTIONS
{
  /* 1. Vector table MUST be at address 0x08000000 */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Prevent linker garbage collection from discarding */
    . = ALIGN(4);
  } >FLASH

  /* 2. Compiled Program Instructions */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* All function code */
    *(.text*)          /* Subsections (-ffunction-sections) */
    *(.glue_7)         /* ARM/Thumb glue code */
    *(.glue_7t)
    *(.eh_frame)
    . = ALIGN(4);
    _etext = .;        /* Define global symbol marking end of code */
  } >FLASH

  /* 3. Read-Only Constants and String Literals */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* const variables, strings (e.g. "Sensor Error\n") */
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* Used by startup assembly to know where to copy .data from Flash */
  _sidata = LOADADDR(.data);

  /* 4. Initialized Variables: VMA is in RAM, LMA is in Flash! */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* Global symbol: start of .data in RAM */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;        /* Global symbol: end of .data in RAM */
  } >RAM AT> FLASH     /* Run in RAM, but store initial image in FLASH! */

  /* 5. Zero-Initialized Variables (Stored ONLY in RAM) */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;         /* Global symbol: start of .bss */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;         /* Global symbol: end of .bss */
    __bss_end__ = _ebss;
  } >RAM

  /* 6. User Stack and Heap Validation Block */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM
}
```

---

### 5.3.2 VMA vs. LMA: The Dual-Address Concept
Notice the directive for the `.data` section:
```ld
} >RAM AT> FLASH
```
This demonstrates the difference between **VMA** and **LMA**:
- **VMA (Virtual Memory Address / Execution Address)**: The address where the variable resides while the program is running (`>RAM`, starting at `0x20000000`). When C code reads `motor_speed`, the CPU generates an SRAM bus read to this address.
- **LMA (Load Memory Address / Storage Address)**: The address where the initial bytes are stored in non-volatile Flash (`AT> FLASH`, tracked by `_sidata`). Because SRAM loses its state when power is turned off, the initial values must be physically saved inside Flash ROM.

---

### 5.3.3 Linker Script Differences on STM32F405: Core Coupled Memory (CCMRAM)
The STM32F405 linker script (`STM32F405RGTX_FLASH.ld`) introduces a dedicated memory block: **CCMRAM (Core Coupled Memory RAM)**:
```ld
MEMORY
{
  CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
  RAM    (xrw): ORIGIN = 0x20000000, LENGTH = 128K
  FLASH  (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
}
```
- **CCMRAM ($64\text{ KB}$ at `0x10000000`)** connects directly to the Cortex-M4 D-bus, providing **$0\text{-wait-state}$ execution with zero bus contention** from other peripherals.
- *Best Used For*: High-speed mathematical buffers (e.g., FFT arrays, Kalman filter state matrices, or interrupt stacks).
- *Critical Hardware Limitation*: **The DMA controller CANNOT access CCMRAM**! If you place an ADC DMA or SPI DMA buffer inside CCMRAM, the transfer will trigger an immediate DMA bus error. DMA buffers must always reside in standard `RAM` (`0x20000000`).

---

## 5.4 Physical Memory Mapping: Where Does Every Variable Live?

To master embedded programming, an engineer must visualize the exact physical memory location of every line of C code.

**Table 5.2 — C variable declaration vs. physical memory placement**

| C Code Declaration Example | Memory Section | Physical Location | Lifetime & Behavior |
|---|---|---|---|
| `const float PI = 3.14159f;` | `.rodata` | **Flash ROM** | Permanent. Never consumes precious SRAM! |
| `const char *msg = "ERROR\n";` | `.rodata` | **Flash ROM** | The string characters reside in Flash. |
| `void Motor_Init(void) { ... }` | `.text` | **Flash ROM** | Machine instruction opcodes stored in Flash. |
| `int motor_speed = 100;` (Global) | `.data` | **SRAM** (Backed by Flash) | Modifiable in SRAM; initialized from Flash at boot. |
| `static int filter_state = 5;` | `.data` | **SRAM** (Backed by Flash) | File-scoped; initialized from Flash at boot. |
| `int sensor_buffer[500];` (Global) | `.bss` | **SRAM** | Zeroed out by `Reset_Handler` at startup. |
| `static float yaw_drift;` | `.bss` | **SRAM** | Initialized to `0.0f` at startup. |
| `void Func(void) { int x = 10; }` | **Stack** | **SRAM** (Top of RAM) | Temporary. Allocated on stack frame; destroyed at function return. |
| `uint8_t *buf = malloc(128);` | **Heap** | **SRAM** (Above `.bss`) | Allocated dynamically at runtime from FreeRTOS or newlib heap. |

```text
       STM32 Physical Memory Address Map
 0x0800 0000 ┌──────────────────────────────────────┐
             │ .isr_vector (Interrupt Vectors)      │
             ├──────────────────────────────────────┤
             │ .text (Program Instructions)         │  FLASH ROM
             ├──────────────────────────────────────┤  (Non-Volatile)
             │ .rodata (Constants & Strings)        │
             ├──────────────────────────────────────┤
             │ .data Initializers (_sidata)         │
 0x0801 0000 └──────────────────────────────────────┘
 
 0x2000 0000 ┌──────────────────────────────────────┐
             │ .data (Initialized Global Variables) │
             ├──────────────────────────────────────┤
             │ .bss (Zero-Initialized Variables)    │
             ├──────────────────────────────────────┤  ON-CHIP SRAM
             │ Heap (malloc, grows downwards ▼)     │  (Volatile)
             │                                      │
             │ Stack (Local vars, grows upwards ▲)  │
 0x2000 5000 └──────────────────────────────────────┘ (_estack = 20 KB limit)
```

---

## 5.5 Map File Auditing: Diagnosing Memory Exhaustion and Linker Errors

When you compile an STM32 project, the linker outputs a master text file: **`ProjectName.map`** (located inside the `Debug/` or `Release/` folder).
The `.map` file is the ultimate source of truth regarding firmware memory utilization.

### 5.5.1 Calculating Exact Memory Consumption
At the conclusion of a build, the linker reports memory usage:
```text
Memory region         Used Size  Region Size  %age Used
           RAM:        4216 B        20 KB     20.59%
         FLASH:       28432 B        64 KB     43.38%
```
The exact mathematical formulas governing these numbers are:

$$\text{Total Flash Used} = \text{sizeof}(.text) + \text{sizeof}(.rodata) + \text{sizeof}(.data)$$
$$\text{Total SRAM Used} = \text{sizeof}(.data) + \text{sizeof}(.bss) + \text{Minimum Stack Size} + \text{Minimum Heap Size}$$

> [!NOTE]
> Notice that **`.data` consumes space in BOTH Flash and SRAM**! It consumes Flash because its initial values must be stored permanently, and it consumes SRAM because its values are modified dynamically at runtime. Conversely, **`.bss` consumes ZERO bytes of Flash**!

---

### 5.5.2 How to Hunt Down Memory Hogs in the `.map` File
Open the `.map` file in STM32CubeIDE and search for the section titled **`Linker script and memory map`**.
Look for the largest symbols sorted by memory consumption:

```text
 .bss.imu_history_buffer
                0x20000410     0x1000  Core/Src/main.o
                0x20000410             imu_history_buffer
```
- In this example, `imu_history_buffer` consumes `0x1000` bytes ($4096\text{ bytes}$), representing over **$20\%$ of the entire $20\text{ KB}$ SRAM of the STM32F103**!

---

### 5.5.3 Diagnosing and Fixing Common Linker Errors

#### Linker Error 1: `region 'RAM' overflowed by XXXX bytes`
- **Root Cause**: The combined size of global arrays (`.bss`), initialized variables (`.data`), and the requested stack/heap exceeds the physical SRAM size defined in the linker script.
- **Engineering Fixes**:
  1. Inspect the `.map` file to identify massive global arrays.
  2. If an array contains constant lookup tables (e.g. sine wave tables or font bitmaps), **add the `const` qualifier**:
     ```c
     /* WRONG: Allocates 2048 bytes in volatile SRAM! */
     uint16_t SINE_LUT[1024] = { ... };

     /* CORRECT: Stored exclusively in 64 KB Flash (.rodata); consumes 0 bytes of RAM! */
     const uint16_t SINE_LUT[1024] = { ... };
     ```
  3. Reduce `_Min_Heap_Size` in `STM32F103C8TX_FLASH.ld` from `0x200` to `0x0` if your firmware does not use `malloc()`.

#### Linker Error 2: `undefined reference to 'xyz'`
- **Root Cause**: The function prototype was declared in a `.h` header file, but the linker cannot find the compiled machine code in any `.o` object file.
- **Common Triggers**:
  1. Forgotten source file: The `.c` file exists in the directory but was not added to the Eclipse build path (Right-click file $\rightarrow$ Resource Configurations $\rightarrow$ Uncheck "Exclude from build").
  2. C++ Name Mangling: Attempting to call a C function from C++ without wrapping the header in `extern "C" { ... }`.
