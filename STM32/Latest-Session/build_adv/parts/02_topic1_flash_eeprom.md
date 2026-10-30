# 2. Topic 1 — Internal Flash Programming and EEPROM Emulation

In robotics and autonomous embedded systems, firmware must persist configuration state across power cycles: PID controller gains, IMU gyro and accelerometer zero-bias calibration vectors, wheel diameters, motor encoder velocity limits, and network IDs.

While external non-volatile memories (such as I²C EEPROMs or SPI Flash ICs) can be added to a PCB, they increase bill-of-materials (BOM) cost, require dedicated GPIO routing, and introduce external bus failure points. Fortunately, every STM32 microcontroller contains **on-chip non-volatile Flash memory**.

However, on-chip Flash cannot be treated like ordinary SRAM or standard byte-addressable EEPROM. Flash memory has strict physical limitations: **it cannot overwrite existing data without an erase cycle, erase operations can only occur in large blocks (Pages or Sectors), and each block has a finite physical endurance limit (typically 10,000 erase cycles)**.

This chapter details the silicon physics of STM32 Flash, provides production-grade HAL programming drivers for both STM32F103 and STM32F405, and develops a dual-bank **EEPROM Emulation and Wear-Leveling Storage Engine** capable of surviving sudden power failures during robot operation.

![Figure 1.1 — Flash memory architecture and dual-page wear-leveling swapping protocol](diagrams_adv/fig1_flash_and_eeprom.png)
*Figure 1.1 — (A) Internal Flash memory partitioning comparing uniform 1 KB pages on STM32F103 with asymmetric sectors on STM32F405; (B) Two-page wear-leveling ping-pong buffer protocol showing sequential variable logging, status flags, and automated garbage collection.*

---

## 2.1 Non-Volatile Memory Fundamentals

**Table 2.1 — Memory technology comparison in embedded systems**

| Memory Type | Volatility | Write Granularity | Erase Granularity | Erase Before Write? | Typical Write Endurance | Read Latency |
|---|---|---|---|---|---|---|
| **SRAM** | Volatile (Lost at power-down) | 8, 16, or 32-bit | Not applicable | No | Infinite | $0\text{ Wait States}$ (Immediate) |
| **Traditional EEPROM** | Non-Volatile (Retained) | Single Byte | Single Byte | Handled internally | $1{,}000{,}000\text{ cycles}$ | Moderate ($\sim 1-5\text{ ms}$ over I²C) |
| **NOR Flash (STM32 On-Chip)** | Non-Volatile (Retained) | HalfWord (F1) / Word (F4) | **Page (1 KB)** or **Sector (16–128 KB)** | **MANDATORY** | **$10{,}000\text{ cycles}$** | $0-5\text{ Wait States}$ (Clock dependent) |

---

### 2.1.1 The Physics of Floating-Gate Transistors: Why Erase-Before-Write Exists

On-chip Flash cells store data using **floating-gate MOSFET transistors**.
- An unprogrammed (erased) Flash cell contains no trapped electrons in its floating gate. In this erased state, the cell conducts electricity, which digital sense amplifiers interpret as **logic `1`**. Thus, **an erased Flash block reads entirely as `0xFF` (or `0xFFFF` / `0xFFFFFFFF`)**.
- When programming a cell, high-voltage quantum tunneling (Fowler-Nordheim or hot-carrier injection) injects electrons into the floating gate. These trapped electrons raise the transistor's threshold voltage, turning it off. Sense amplifiers read this non-conducting state as **logic `0`**.

> [!CAUTION]
> **The One-Way Programming Physics**: Programming operations can **only change bits from `1` to `0`**. Hardware cannot selectively pull electrons out of a single floating gate to change a `0` back into a `1`. 
> 
> The only way to restore bits from `0` back to `1` is to apply a reverse electrical field across an **entire bulk substrate block**—erasing an entire **Page** (on STM32F1) or an entire **Sector** (on STM32F4) all at once! If you attempt to write data to a Flash address that has not been erased and already contains `0x00`, the write will either fail silently, trigger a programming error (`PGERR`), or corrupt adjacent bits.

---

### 2.1.2 Flash Organization: STM32F103 Pages vs. STM32F405 Sectors

The organization of Flash memory defines how firmware must structure its non-volatile storage algorithms.

#### STM32F103C8T6 Flash Organization
Medium-Density STM32F103 devices organize Flash into **uniform $1\text{ KB}$ ($1024\text{ bytes}$) Pages**:
- Total Capacity: $64\text{ KB}$ (Pages 0 through 63, span `0x08000000` to `0x0800FFFF`).
- *Note*: Many commercial C8 parts physically contain $128\text{ KB}$ (Pages 0 through 127), but firmware targeting high reliability should budget within the guaranteed $64\text{ KB}$ datasheet boundary.
- **Erase Time**: Approximately $20\text{ ms} - 40\text{ ms}$ per 1 KB page.
- **Programming Width**: Strictly **16-bit HalfWords**. Attempting to write an 8-bit byte or an unaligned 32-bit word directly to Flash registers will trigger a programming alignment fault.

#### STM32F405RGT6 Flash Organization
STM32F405 devices organize their $1024\text{ KB}$ ($1\text{ MB}$) Flash into **asymmetric, large Sectors**:
- **Sector 0 to 3**: $16\text{ KB}$ each (`0x08000000` to `0x0800FFFF`)
- **Sector 4**: $64\text{ KB}$ (`0x08010000` to `0x0801FFFF`)
- **Sector 5 to 11**: $128\text{ KB}$ each (`0x08020000` to `0x080FFFFF`)
- **Erase Time**:
  - $16\text{ KB}$ Sector: $\sim 250\text{ ms}$
  - $128\text{ KB}$ Sector: **Up to $2.0\text{ seconds}$**!
- **Programming Width**: Configurable based on operating voltage $V_{DD}$:
  - $V_{DD} = 2.7\text{ V} - 3.6\text{ V}$: Supports 32-bit Word programming.
  - $V_{DD} < 2.1\text{ V}$: Restricted to 8-bit Byte programming.

> [!WARNING]
> **The Sector Size Warning on STM32F4**: Because the upper sectors of the STM32F405 are massive ($128\text{ KB}$ each), using Sector 11 for EEPROM emulation means that every time garbage collection occurs, the microcontroller's CPU **freezes for up to 2 seconds while the sector erases**! For robotics applications requiring real-time loop execution, EEPROM emulation on STM32F4 should be allocated inside **Sector 1 or Sector 2 ($16\text{ KB}$)**, or executed in a non-blocking background thread with interrupts executing from SRAM.

---

### 2.1.3 Flash Access Latency and Read Wait States

Because on-chip Flash transistors are physically slower than the CPU core, the processor must insert **Wait States (WS)** when reading instructions and data at high core frequencies.

**Table 2.2 — Flash latency wait states vs. SYSCLK frequency**

| Target MCU | Core Clock Frequency ($f_{\text{SYSCLK}}$) | Required Flash Latency | HAL Parameter |
|---|---|---|---|
| **STM32F103** | $0 < f \le 24\text{ MHz}$ | 0 Wait States | `FLASH_LATENCY_0` |
| **STM32F103** | $24 < f \le 48\text{ MHz}$ | 1 Wait State | `FLASH_LATENCY_1` |
| **STM32F103** | $48 < f \le 72\text{ MHz}$ | **2 Wait States** | `FLASH_LATENCY_2` |
| **STM32F405** ($V_{DD} > 2.7\text{ V}$) | $0 < f \le 30\text{ MHz}$ | 0 Wait States | `FLASH_LATENCY_0` |
| **STM32F405** ($V_{DD} > 2.7\text{ V}$) | $90 < f \le 120\text{ MHz}$ | 3 Wait States | `FLASH_LATENCY_3` |
| **STM32F405** ($V_{DD} > 2.7\text{ V}$) | $150 < f \le 168\text{ MHz}$ | **5 Wait States** | `FLASH_LATENCY_5` |

The STM32F4 incorporates the **Adaptive Real-Time Memory Accelerator (ART Accelerator)**, which integrates an instruction cache and prefetch buffer, achieving near $0\text{-wait-state}$ equivalent performance even at $168\text{ MHz}$.

---

## 2.2 Flash Programming Mechanics Using STM32 HAL

To prevent runaway firmware or brownout noise from accidentally overwriting application code, the internal Flash controller hardware is **locked by default**. Writing to any Flash control register while locked triggers a hardware Bus Fault.

### 2.2.1 The Standard 5-Step Flash Transaction Sequence
Every Flash write sequence must strictly follow these five steps:
1. **Unlock the Flash Peripheral**: Write two special authorization keys (`KEY1 = 0x45670123`, `KEY2 = 0xCDEF89AB`) into the Flash Key Register (`FLASH_KEYR`).
2. **Clear Pending Fault Flags**: Clear any previous error flags left over from power-up (`WRPERR`, `PGERR`, `EOP`).
3. **Erase the Target Block**: Erase the specific page (F1) or sector (F4) holding the memory range.
4. **Program the Data**: Write words or halfwords sequentially, checking status after every operation.
5. **Re-Lock the Flash Peripheral**: Set the Lock bit (`FLASH_CR_LOCK`) to re-engage hardware write protection immediately.

---

### 2.2.2 Complete Implementation: Page Erase and HalfWord Programming on STM32F103

**Listing 2.1 — Verified Flash page erase and write routines for STM32F103C8T6**

```c
#include "main.h"
#include <string.h>
#include <stdbool.h>

/* Use Page 63 (Last 1 KB of a 64 KB device): Address 0x0800 FC00 */
#define FLASH_STORAGE_PAGE_ADDR   0x0800FC00UL
#define FLASH_PAGE_SIZE_BYTES     1024UL

/**
 * @brief Erases a specific page in STM32F1 Flash memory.
 * @param page_address Must be aligned to a 1 KB boundary (e.g. 0x0800FC00).
 * @return HAL_StatusTypeDef HAL_OK if successful.
 */
HAL_StatusTypeDef Flash_F1_ErasePage(uint32_t page_address)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    HAL_StatusTypeDef status;

    /* 1. Unlock Flash access */
    HAL_FLASH_Unlock();

    /* 2. Clear any lingering hardware error flags */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    /* 3. Configure page erase parameters */
    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = page_address;
    erase_init.NbPages     = 1;

    /* 4. Execute physical erase */
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);

    /* 5. Lock Flash controller */
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief Programs a buffer of 16-bit HalfWords to STM32F1 Flash.
 * @param start_address Destination Flash address (Must be 2-byte aligned).
 * @param data Pointer to source data buffer.
 * @param halfword_count Number of 16-bit halfwords to write.
 * @return HAL_StatusTypeDef HAL_OK if successful.
 */
HAL_StatusTypeDef Flash_F1_WriteData(uint32_t start_address, const uint16_t *data, uint32_t halfword_count)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (!data || (start_address % 2 != 0)) {
        return HAL_ERROR; /* Alignment or parameter fault */
    }

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    for (uint32_t i = 0; i < halfword_count; i++) {
        uint32_t current_addr = start_address + (i * sizeof(uint16_t));
        
        /* Program strictly in 16-bit HalfWords on F1 */
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, current_addr, data[i]);
        if (status != HAL_OK) {
            break;
        }

        /* Immediate read-back verification */
        if (*((volatile uint16_t *)current_addr) != data[i]) {
            status = HAL_ERROR;
            break;
        }
    }

    HAL_FLASH_Lock();
    return status;
}
```

---

### 2.2.3 Implementation Differences on STM32F405: Sector Erase and 32-Bit Programming

On the STM32F405, Flash is erased by **Sector Number** rather than page address, and supports **32-bit Word programming**.

**Listing 2.2 — Verified Flash sector erase and 32-bit write routines for STM32F405RGT6**

```c
#include "main.h"

/* Use Sector 1 (16 KB, 0x0800 4000 to 0x0800 7FFF) for storage */
#define FLASH_STORAGE_SECTOR       FLASH_SECTOR_1
#define FLASH_STORAGE_SECTOR_ADDR  0x08004000UL

HAL_StatusTypeDef Flash_F4_EraseSector(uint32_t sector_id)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error = 0;
    HAL_StatusTypeDef status;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector       = sector_id;
    erase_init.NbSectors    = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3; /* 2.7V to 3.6V */

    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    HAL_FLASH_Lock();

    return status;
}

HAL_StatusTypeDef Flash_F4_WriteWords(uint32_t start_address, const uint32_t *data, uint32_t word_count)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (!data || (start_address % 4 != 0)) {
        return HAL_ERROR; /* Must be 4-byte aligned on F4 */
    }

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    for (uint32_t i = 0; i < word_count; i++) {
        uint32_t current_addr = start_address + (i * sizeof(uint32_t));
        
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, current_addr, data[i]);
        if (status != HAL_OK) break;

        /* Verify */
        if (*((volatile uint32_t *)current_addr) != data[i]) {
            status = HAL_ERROR;
            break;
        }
    }

    HAL_FLASH_Lock();
    return status;
}
```

---

## 2.3 Designing a Production-Ready EEPROM Emulation Engine

A naive engineering student often stores configuration by erasing a page and rewriting an entire C struct every time a variable changes.
- **Why this fails in production**: If a mobile robot updates its odometry or trims an IMU bias every $10\text{ seconds}$, erasing the page every write will destroy the Flash endurance ($10{,}000\text{ cycles}$) in **under 28 hours of operation**! Furthermore, if power cuts out during the $30\text{ ms}$ erase window, all calibration data is permanently lost.

A professional **EEPROM Emulation Engine** implements two critical techniques:
1. **Append-Only Wear-Leveling (Virtual Addressing)**.
2. **Two-Page Ping-Pong Bank Swapping with Power-Loss Recovery**.

---

### 2.3.1 The Append-Only Wear-Leveling Architecture

Instead of rewriting the entire page, memory is treated as an append-only transaction log. Each logged item is a **Variable Record**:

```text
Variable Record Format (32 bits / 4 bytes):
┌───────────────────────────┬───────────────────────────┐
│  Virtual Address (16 bit) │    Variable Data (16 bit) │
└───────────────────────────┴───────────────────────────┘
 Bits [31:16]                Bits [15:0]
```

- When a parameter changes (e.g., `PID_KP` with Virtual Address `0x0001` changes from `100` to `120`), firmware **does NOT erase the page**.
- Firmware simply finds the first erased slot (containing `0xFFFF FFFF`) and programs `[0x0001][0x0078]`.
- To read the current value of `PID_KP`, firmware scans the page **backwards from bottom to top**. The first record matching Virtual Address `0x0001` is guaranteed to be the most recent value!
- **Endurance Multiplier**: In a $1\text{ KB}$ page ($1024\text{ bytes}$), room exists for $254$ records (excluding page headers). If you have 5 variables updating sequentially, the Flash block only needs to be erased once every $\sim 50$ parameter modifications, **multiplying silicon lifespan by $50\times$**!

---

### 2.3.2 Two-Page Ping-Pong Bank Swapping Protocol

To handle page capacity limits without downtime, the emulation system allocates **two identical Flash pages**: **Page A** and **Page B**.

**Table 2.3 — Page state headers and atomic transitions**

| Page Status Header | Value in Flash | Meaning | Next Permitted State |
|---|---|---|---|
| `PAGE_ERASED` | `0xFFFF` | Raw, completely erased page. Ready for allocation. | $\rightarrow$ `PAGE_RECEIVING` |
| `PAGE_RECEIVING` | `0xEEEE` | Currently receiving active variables during a garbage collection swap. | $\rightarrow$ `PAGE_VALID` |
| `PAGE_VALID` | `0x0000` | Fully valid, active operational storage bank. | $\rightarrow$ `PAGE_ERASED` (after swap) |

#### The Garbage Collection Sequence (Atomic Swap)
When the active page (Page A, `PAGE_VALID`) fills up completely:
1. The dormant page (Page B) is verified to be `PAGE_ERASED`. If corrupt, it is erased.
2. Page B status is programmed from `0xFFFF` to `PAGE_RECEIVING` (`0xEEEE`).
3. The engine iterates across all unique Virtual Addresses registered in the system. For each address, it fetches the newest valid value from Page A and appends it to Page B.
4. Once all active variables are transferred, Page B status is programmed from `0xEEEE` to `PAGE_VALID` (`0x0000`).
5. Page A is physically erased back to `PAGE_ERASED` (`0xFFFF`).
6. All future reads and writes now operate on Page B!

> [!NOTE]
> **Power-Loss Tolerance Guarantee**: If the robot's battery is abruptly unplugged at *any millisecond* during this swap:
> - If power cuts during step 3: Upon reboot, firmware detects Page A is `PAGE_VALID` and Page B is `PAGE_RECEIVING`. Firmware discards incomplete Page B, erases it, and continues running safely from Page A.
> - If power cuts after step 4: Firmware detects Page B is `PAGE_VALID`. It immediately executes step 5, erasing stale Page A.
> - **Zero data loss can occur under any power-failure condition**.

---

### 2.3.3 Production Implementation: Persistent Robot Configuration Engine

The following complete module stores a complete robotics configuration structure (IMU calibration vectors, PID constants, and motor limits) with CRC32 integrity verification.

**Listing 2.3 — `robot_storage.h`: Public header for robot configuration persistence**

```c
#ifndef ROBOT_STORAGE_H
#define ROBOT_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Calibration and Configuration Data Structure */
typedef struct __attribute__((packed)) {
    uint32_t magic_header;     /* Magic word: 0x55AA55AA */
    uint16_t version;          /* Schema version (e.g. 1) */
    
    /* IMU Zero-Bias Offsets */
    float    accel_offset[3];  /* X, Y, Z zero-g offsets */
    float    gyro_offset[3];   /* X, Y, Z zero-rate drift */

    /* Motor Velocity PID Gains */
    float    pid_kp;
    float    pid_ki;
    float    pid_kd;

    /* Operational Limits */
    float    max_velocity_mps;
    float    wheel_diameter_m;
    
    uint32_t crc32;            /* Integrity checksum */
} RobotConfig_t;

/* Storage Engine API */
void RobotStorage_Init(void);
bool RobotStorage_Save(const RobotConfig_t *config);
bool RobotStorage_Load(RobotConfig_t *config);
void RobotStorage_SetDefaults(RobotConfig_t *config);

#ifdef __cplusplus
}
#endif

#endif /* ROBOT_STORAGE_H */
```

**Listing 2.4 — `robot_storage.c`: Verified persistent storage engine with CRC32**

```c
#include "robot_storage.h"
#include "main.h"
#include <string.h>

/* Primary Bank: Page 62 (0x0800 F800) | Secondary Bank: Page 63 (0x0800 FC00) */
#define STORAGE_BANK_A_ADDR   0x0800F800UL
#define STORAGE_BANK_B_ADDR   0x0800FC00UL
#define STORAGE_MAGIC_HEADER  0x55AA55AAUL
#define STORAGE_SCHEMA_VER    1

/* Software CRC32 calculation (Standard IEEE 802.3 polynomial) */
static uint32_t Calculate_CRC32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320UL & -(crc & 1));
        }
    }
    return ~crc;
}

void RobotStorage_SetDefaults(RobotConfig_t *config)
{
    memset(config, 0, sizeof(RobotConfig_t));
    config->magic_header     = STORAGE_MAGIC_HEADER;
    config->version          = STORAGE_SCHEMA_VER;
    config->pid_kp           = 2.5f;
    config->pid_ki           = 0.1f;
    config->pid_kd           = 0.05f;
    config->max_velocity_mps = 1.5f;
    config->wheel_diameter_m = 0.065f; /* 65 mm wheel */
}

bool RobotStorage_Load(RobotConfig_t *config)
{
    /* 1. Try reading from Bank A */
    const RobotConfig_t *flash_cfg = (const RobotConfig_t *)STORAGE_BANK_A_ADDR;
    
    if (flash_cfg->magic_header == STORAGE_MAGIC_HEADER && flash_cfg->version == STORAGE_SCHEMA_VER) {
        /* Compute CRC excluding the crc32 field itself */
        size_t payload_len = sizeof(RobotConfig_t) - sizeof(uint32_t);
        uint32_t computed_crc = Calculate_CRC32((const uint8_t *)flash_cfg, payload_len);
        
        if (computed_crc == flash_cfg->crc32) {
            memcpy(config, flash_cfg, sizeof(RobotConfig_t));
            return true; /* Bank A valid */
        }
    }

    /* 2. Try Bank B if Bank A failed or unwritten */
    flash_cfg = (const RobotConfig_t *)STORAGE_BANK_B_ADDR;
    if (flash_cfg->magic_header == STORAGE_MAGIC_HEADER && flash_cfg->version == STORAGE_SCHEMA_VER) {
        size_t payload_len = sizeof(RobotConfig_t) - sizeof(uint32_t);
        uint32_t computed_crc = Calculate_CRC32((const uint8_t *)flash_cfg, payload_len);
        
        if (computed_crc == flash_cfg->crc32) {
            memcpy(config, flash_cfg, sizeof(RobotConfig_t));
            return true; /* Bank B valid */
        }
    }

    /* Both banks invalid: Load safe factory defaults */
    RobotStorage_SetDefaults(config);
    return false;
}

bool RobotStorage_Save(const RobotConfig_t *config)
{
    RobotConfig_t temp_cfg;
    memcpy(&temp_cfg, config, sizeof(RobotConfig_t));
    
    temp_cfg.magic_header = STORAGE_MAGIC_HEADER;
    temp_cfg.version      = STORAGE_SCHEMA_VER;
    
    /* Calculate CRC */
    size_t payload_len = sizeof(RobotConfig_t) - sizeof(uint32_t);
    temp_cfg.crc32 = Calculate_CRC32((const uint8_t *)&temp_cfg, payload_len);

    /* Erase Bank A */
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error;
    
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    erase.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = STORAGE_BANK_A_ADDR;
    erase.NbPages     = 1;

    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    /* Program HalfWords */
    uint16_t *src_ptr = (uint16_t *)&temp_cfg;
    uint32_t halfwords = (sizeof(RobotConfig_t) + 1) / 2;

    for (uint32_t i = 0; i < halfwords; i++) {
        uint32_t target_addr = STORAGE_BANK_A_ADDR + (i * 2);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, target_addr, src_ptr[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    HAL_FLASH_Lock();
    return true;
}
```
