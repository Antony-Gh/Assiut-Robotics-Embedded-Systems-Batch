# 8. Topic 7 — Custom Bootloaders and In-System Programming

During initial development, an engineer flashes firmware into an STM32 using an ST-LINK hardware programmer connected via Serial Wire Debug (SWD).

However, once a robot is assembled, enclosed inside an aluminum chassis, or deployed in the field at a customer site, **accessing SWD header pins with an ST-LINK probe is often physically impossible**. Firmware must be updated over existing communication channels: a USB port, an RS-485 bus, a CAN bus, a wireless telemetry radio, or an Ethernet link.

Enabling field firmware updates requires a **Custom Bootloader**.

A bootloader is a dedicated, self-contained firmware program residing at the base of on-chip Flash. Upon reset, the bootloader initializes basic communication peripherals, validates whether a valid application is present, listens for incoming firmware update binaries, writes new code into Flash, and seamlessly transfers execution to the user application.

This chapter details the STM32 factory ROM bootloader, develops the architecture of a custom two-stage bootloader, formulates vector table relocation rules using the **Vector Table Offset Register (`SCB->VTOR`)**, implements a verified C application jump sequence, and examines cryptographic firmware integrity verification.

![Figure 7.1 — Custom bootloader Flash partitioning, metadata headers, and application jump sequence](diagrams_adv/fig7_bootloader_and_vtor.png)
*Figure 7.1 — (A) On-chip Flash memory partitioning comparing Bootloader (16 KB), Metadata Header (1 KB), and User Application regions; (B) Step-by-step application jump flowchart showing stack pointer validation, peripheral deinitialization, VTOR relocation, and Reset_Handler branch.*

---

## 8.1 Bootloader Fundamentals: Dual-Stage Booting

In modern embedded systems, booting is structured as a multi-stage hierarchy:

```text
 Silicon Power-On Reset
          │
          ▼
 [Stage 1: Hardware Boot Configuration] ──► Checks BOOT0 / BOOT1 pins
          │
          ├──► BOOT0 = 1 ──► [STM32 Factory ROM Bootloader] (USART1 / USB DFU)
          │
          └──► BOOT0 = 0 ──► [Stage 2: Custom Application Bootloader] (Base 0x0800 0000)
                                    │
                                    ├── Update Requested? ──► [Download & Flash New Firmware]
                                    │
                                    └── Valid Application? ──► [Jump to User Application]
```

### Why Write a Custom Bootloader Instead of Using Factory ROM?
Every STM32 contains a factory ROM bootloader written by STMicroelectronics. While useful for initial factory programming, the factory bootloader has severe limitations:
1. **Physical Pin Manipulation**: Entering factory boot mode requires driving the **`BOOT0` pin HIGH** and power-cycling the board. An end-user cannot be expected to open an enclosure and set physical jumpers.
2. **Fixed Protocol & Pins**: The factory bootloader only communicates over fixed hardware pins (e.g., USART1 on PA9/PA10). If your robot communicates over USART2, CAN bus, or a custom wireless protocol, the factory ROM bootloader cannot be used.
3. **Zero Security & Authentication**: The factory bootloader does not verify cryptographic signatures or perform fail-safe rollbacks if an update is interrupted mid-write.

---

## 8.2 The STM32 Factory ROM Bootloader

When `BOOT0 = 1` and `BOOT1 = 0` during reset, the ARM core boots from **System Memory**:
- **STM32F103 Address**: `0x1FFFF000` (Contains ST's pre-programmed factory ROM).
  - *Supported Interfaces on F103*: Strictly **USART1** (Pins `PA9` / `PA10`).
- **STM32F405 Address**: `0x1FFF0000`.
  - *Supported Interfaces on F405*: **USART1**, **USART3**, **CAN2**, and native **USB DFU (Device Firmware Upgrade)**.

The factory bootloader implements the proprietary **ST Bootloader Protocol (AN3155 / AN3156)**, supporting commands like `Get` (`0x00`), `Read Memory` (`0x11`), `Go` (`0x21`), `Write Memory` (`0x31`), and `Erase` (`0x43`). The open-source `stm32flash` CLI tool and ST's official `STM32CubeProgrammer` software utilize this protocol.

---

## 8.3 Custom Bootloader Memory Architecture

A custom bootloader divides on-chip Flash memory into three logical partitions.

**Table 8.1 — Flash partitioning scheme for STM32F103C8T6 (64 KB Flash)**

| Memory Region | Address Range | Allocated Size | Purpose & Functionality |
|---|---|---|---|
| **Bootloader Firmware** | `0x0800 0000` to `0x0800 3FFF` | **16 KB** (Pages 0–15) | Core bootloader logic, UART/CAN communications, Flash writer. |
| **Application Metadata Header**| `0x0800 4000` to `0x0800 43FF` | **1 KB** (Page 16) | Stores Magic Word, version, firmware size, and CRC32 checksum. |
| **User Application Firmware** | `0x0800 4400` to `0x0800 FFFF` | **47 KB** (Pages 17–63)| Main robotics firmware. Vector table compiled at `0x0800 4400`. |

```text
 Flash Base Address: 0x0800 0000
 0x0800 0000 ┌──────────────────────────────────────┐
             │ Bootloader Vector Table & Code       │  BOOTLOADER REGION
             │ (Executes immediately on reset)      │  (16 KB)
 0x0800 4000 ├──────────────────────────────────────┤
             │ Application Metadata Header          │  METADATA PAGE
             │ (Magic Word, Size, CRC32, Version)   │  (1 KB)
 0x0800 4400 ├──────────────────────────────────────┤
             │ Application Vector Table             │
             │ Application Code (.text, .rodata)    │  USER APPLICATION
             │ Application Data Initializers        │  (47 KB)
 0x0801 0000 └──────────────────────────────────────┘
```

---

### 8.3.1 Application Metadata Header
To prevent jumping into blank or corrupt Flash (which causes immediate HardFaults), the bootloader checks an **Application Header** programmed alongside the firmware image:

```c
typedef struct __attribute__((packed)) {
    uint32_t magic_word;      /* Must be exactly 0x55AA55AA */
    uint32_t firmware_size;   /* Size of application binary in bytes */
    uint32_t crc32;           /* CRC32 of application binary */
    uint16_t version_major;   /* Firmware Version: e.g. 2 */
    uint16_t version_minor;   /* Firmware Version: e.g. 1 */
} AppHeader_t;
```

---

## 8.4 The Interrupt Vector Table Offset Register (`SCB->VTOR`)

Understanding the **Vector Table Offset Register (`SCB->VTOR`)** is the single most critical concept in bootloader engineering.

### 8.4.1 The Vector Table Problem
When an interrupt fires (e.g., a timer update or UART byte arrival), the ARM Cortex-M hardware looks up the corresponding ISR function pointer inside the **Interrupt Vector Table**.
- By default upon reset, the vector table is located at address **`0x00000000`** (which maps to Flash base `0x08000000`).
- The bootloader has its own vector table at `0x08000000`.
- The user application has its **OWN, completely separate vector table compiled at `0x08004400`**.

If the bootloader jumps to the application without updating `SCB->VTOR`:
- When an application timer interrupt fires, **the CPU core fetches the ISR pointer from the BOOTLOADER'S vector table at `0x08000000`**!
- The CPU branches into the wrong code, immediately triggering a fatal `HardFault` or restarting the bootloader!

---

### 8.4.2 The Solution: Relocating `SCB->VTOR`
The ARM Cortex-M System Control Block provides `SCB->VTOR`:

$$\text{SCB->VTOR} = \text{APPLICATION\_ADDRESS}$$

```c
/* Inform the ARM core that interrupt vectors are now at address 0x0800 4400 */
SCB->VTOR = 0x08004400UL;
```

> [!CAUTION]
> **Vector Table Alignment Constraint**: According to the *ARMv7-M Architecture Reference Manual*, the address loaded into `SCB->VTOR` **must be strictly aligned to a power-of-two boundary matching the total size of the vector table**.
> - On STM32F103 (with $\sim 60$ interrupt vectors $\times 4\text{ bytes} \approx 240\text{ bytes}$), the table must be aligned to at least a **$128\text{-byte}$ or $256\text{-byte}$ boundary**.
> - On STM32F405 (with $\sim 100$ vectors), it must be aligned to a **$512\text{-byte}$ boundary**.
> - `0x08004400` is evenly divisible by $1024$ ($0x400$), fully satisfying all Cortex-M alignment rules.

---

## 8.5 The 8-Step Application Jump Sequence

Jumping from a bootloader to a user application requires cleaning up the microcontroller state so the application inherits a pristine execution environment.

**Listing 8.1 — Production C implementation of the application jump sequence**

```c
#include "main.h"

#define APPLICATION_ADDRESS   0x08004400UL

/* Function pointer prototype for the application entry point */
typedef void (*pFunction)(void);

/**
 * @brief Safely de-initializes the bootloader and transfers execution to the application.
 */
void Bootloader_JumpToApplication(void)
{
    /* Step 1: Validate that the application space contains valid code */
    /* Check that the application's initial Stack Pointer resides within valid SRAM (0x2000 0000) */
    uint32_t app_stack_pointer = *(volatile uint32_t *)APPLICATION_ADDRESS;
    if ((app_stack_pointer & 0x2FFE0000UL) != 0x20000000UL) {
        /* Invalid stack pointer: Application is corrupt or Flash is erased! */
        return;
    }

    /* Step 2: Disable all global interrupts */
    __disable_irq();

    /* Step 3: Disable SysTick timer and clear pending tick flags */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* Step 4: De-initialize all peripherals and reset HAL state */
    HAL_DeInit();

    /* Step 5: Clear all pending interrupt requests in NVIC */
    for (int i = 0; i < 8; i++) {
        NVIC->ICPR[i] = 0xFFFFFFFFUL; /* Clear pending IRQs */
    }

    /* Step 6: Relocate Vector Table to the application start address */
    SCB->VTOR = APPLICATION_ADDRESS;

    /* Step 7: Set the Main Stack Pointer (MSP) to the application's initial stack */
    __set_MSP(app_stack_pointer);

    /* Step 8: Fetch application Reset_Handler address (Second word at APPLICATION_ADDRESS + 4) */
    uint32_t app_reset_handler_addr = *(volatile uint32_t *)(APPLICATION_ADDRESS + 4);
    pFunction app_entry = (pFunction)app_reset_handler_addr;

    /* Re-enable global interrupts before jumping */
    __enable_irq();

    /* Execute branch to Application Reset_Handler! */
    app_entry();

    /* Execution should NEVER reach here */
    while (1);
}
```

---

## 8.6 Configuring the User Application Project to Run with a Bootloader

A standard STM32 project generated by CubeMX is configured to run at Flash base `0x08000000`. To run under a bootloader at `0x08004400`, the **application project must be modified in two places**:

### 1. Modify the Application Linker Script (`STM32F103C8TX_FLASH.ld`)
Change the `FLASH` origin and length:

```ld
/* ORIGINAL CubeMX configuration: */
/* FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K */

/* MODIFIED for Bootloader Compatibility: */
MEMORY
{
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
  FLASH (rx)  : ORIGIN = 0x08004400, LENGTH = 47K   /* Start at 0x0800 4400, Size = 47 KB */
}
```

### 2. Set `SCB->VTOR` in Application `system_stm32f1xx.c`
In the application project's `system_stm32f1xx.c`, set the vector offset:

```c
#define VECT_TAB_OFFSET  0x00004400U /* Matches the 0x0800 4400 Flash offset */
```
When `SystemInit()` executes, it writes `SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET`, guaranteeing that application interrupts function correctly.
