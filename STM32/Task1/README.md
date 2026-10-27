# STM32 Task 1 — Full Solution

**Assiut Robotics Team — Embedded Systems Batch**  
**Target MCU:** STM32F103C8T6 (ARM Cortex-M3 "Blue Pill")  
**Project Path:** `STM32/Task1/STM32F103C8TX/STM32F103C8TX`  
**Reference Assignment:** [`Task1-STM.pdf`](./Task1-STM.pdf)  
**Reference Lecture:** Eng. Ahmed Eldeeb — ARM Course (الدرس 8: GPIO Driver & Registers)  
**Reference Manual:** ST RM0008 Reference Manual  

---

## 1. Objectives & Deliverables Summary

All requirements from `Task1-STM.pdf` are fully implemented inside this **single STM32 project**:

| Deliverable | Requirement | Implementation in Project |
| :--- | :--- | :--- |
| **1. Snake Effect** | Connect 8 LEDs; light them one-by-one, then switch them off one-by-one. | Implemented in **both** STM32 HAL (`Task1_SnakeEffect_HAL_Step`) and direct **Register-Level** (`Task1_SnakeEffect_Register_Step`). |
| **2. 50Hz PWM Signal** | Generate a PWM signal with frequency $f = 50\text{ Hz}$ ($T = 20\text{ ms}$). | Implemented in **Hardware Timer 4 Channel 3** (Register-level, pin `PB8`) and **Software Toggle** (pin `PA0`). Also features LED fading and servo sweep demonstrations. |
| **3. Controlled Counter** | Controlled counter with switch on the same 8 LEDs that represents numbers from $0$ to $8$. | Push button on `PB0` increments counter from $0$ to $8$ (wraps to $0$). Displays in **Bar-Graph style** (0 to 8 LEDs lit) or **Binary style**. Implemented in both HAL and Register level. |
| **Note: Register Level** | Select one task and make it in register level. | **All three deliverables** are implemented at the Register Level (`RCC`, `CRL`, `CRH`, `ODR`, `IDR`, `BSRR`, `TIM4`) alongside HAL implementations. |

---

## 2. Hardware Wiring & Pinout Guide

### Pin Connections

| Peripheral / Signal | STM32 Pin | Mode | Description |
| :--- | :--- | :--- | :--- |
| **LED 0** | `PA0` | Output Push-Pull | First LED of the 8-LED bar |
| **LED 1** | `PA1` | Output Push-Pull | Second LED |
| **LED 2** | `PA2` | Output Push-Pull | Third LED |
| **LED 3** | `PA3` | Output Push-Pull | Fourth LED |
| **LED 4** | `PA4` | Output Push-Pull | Fifth LED |
| **LED 5** | `PA5` | Output Push-Pull | Sixth LED |
| **LED 6** | `PA6` | Output Push-Pull | Seventh LED |
| **LED 7** | `PA7` | Output Push-Pull | Eighth LED |
| **Counter Switch (SW1)** | `PB0` | Input with Pull-Down | Increments counter $0 \to 8 \to 0$ (Debounced, EXTI0 + Polling) |
| **Mode Switch (SW2)** | `PB1` | Input with Pull-Down | Cycles active task mode (EXTI1 + Polling) |
| **50Hz Hardware PWM Output** | `PB8` | Alternate Function PP | TIM4 Channel 3 ($f = 50\text{ Hz}$, $T = 20\text{ ms}$, continuous output) |

```
                       STM32F103C8T6 (Blue Pill)
                       +-----------------------+
                       |                       |
               LED 0 <-| PA0               PB0 |<- Push Button 1 (Counter)
               LED 1 <-| PA1               PB1 |<- Push Button 2 (Mode Select)
               LED 2 <-| PA2                   |
               LED 3 <-| PA3               PB8 |-> 50Hz PWM Output (TIM4_CH3)
               LED 4 <-| PA4                   |
               LED 5 <-| PA5                   |
               LED 6 <-| PA6                   |
               LED 7 <-| PA7                   |
                       +-----------------------+
```

> **Note on Resistors:**  
> Connect each LED through a $220\Omega$ to $330\Omega$ current-limiting resistor to Ground (Active-High).  
> Push buttons on `PB0` and `PB1` are connected between $+3.3\text{V}$ and the pin, using the STM32 internal pull-down resistors (or external $10\text{k}\Omega$ pulldown).

---

## 3. Operational Modes & Interactive System

The firmware features an interactive mode system that allows cycling through all deliverables live on hardware without reflashing:

### Active Modes (`Task1_Mode_t`):
1. **Mode 0 (`MODE_ALL_IN_ONE`) [Default]:**  
   - Background: Hardware 50Hz PWM runs continuously on `PB8`.  
   - Foreground: The 8 LEDs perform the Snake Effect. Pressing `PB0` steps the counter.
2. **Mode 1 (`MODE_SNAKE_HAL`):**  
   - Snake Effect running purely via STM32 HAL API (`HAL_GPIO_WritePin`).
3. **Mode 2 (`MODE_SNAKE_REG`):**  
   - Snake Effect running purely via Direct Register Access (`GPIOA->ODR`).
4. **Mode 3 (`MODE_COUNTER_HAL`):**  
   - Controlled counter ($0$ to $8$) using HAL API. Each press of `PB0` advances count.
5. **Mode 4 (`MODE_COUNTER_REG`):**  
   - Controlled counter ($0$ to $8$) using Direct Register Access (`GPIOA->ODR`).
6. **Mode 5 (`MODE_PWM_50HZ_DEMO`):**  
   - 50Hz PWM breathing LED demonstration on the 8 LEDs and oscilloscope signal on `PB8` / `PA0`.

### Mode Switching Controls:
- **Two-Button Hardware:** Press **PB1** to cycle through the modes.  
- **Single-Button Hardware:** Long-press **PB0** ($> 1\text{ second}$) to cycle through the modes; short-press increments the counter.  
- **Compile-Time Selection:** In `Core/Src/main.c`, change `#define DEFAULT_STARTUP_MODE` to any specific mode if desired.

---

## 4. Deep-Dive: Register-Level Implementation (Eng. Ahmed Eldeeb — Lesson 8)

According to the STM32F103 Reference Manual (RM0008) and Eng. Ahmed Eldeeb's course:

### 1. Clock Enable (`RCC->APB2ENR` & `RCC->APB1ENR`)
In ARM Cortex-M microcontrollers, all peripheral clocks are disabled by default to save power. Before accessing any GPIO or Timer register, its peripheral clock must be enabled:
```c
/* Enable Port A, Port B, and AFIO on APB2 */
RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN);

/* Enable Timer 4 on APB1 */
RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
```

### 2. GPIO Port Configuration Register Low (`GPIOA->CRL`)
Controls pins 0 through 7 (our 8 LEDs). Each pin is configured using 4 bits: `CNF[1:0] : MODE[1:0]`.
- For Output Push-Pull at 2 MHz:
  - `MODE[1:0] = 10b` (Output mode, max speed 2 MHz)
  - `CNF[1:0]  = 00b` (General purpose output push-pull)
  - Resulting 4-bit nibble: `0010b` = `0x2`.
- Since all 8 pins (`PA0`..`PA7`) are identical outputs:
```c
GPIOA->CRL = 0x22222222; /* Configures PA0..PA7 as Push-Pull Output (2MHz) */
```

### 3. Switch Configuration with Internal Pull-Down (`GPIOB->CRL` & `GPIOB->ODR`)
- For Input with Pull-Up/Pull-Down:
  - `MODE[1:0] = 00b` (Input mode)
  - `CNF[1:0]  = 10b` (Input with pull-up / pull-down)
  - Resulting nibble: `1000b` = `0x8`.
- Writing `0` to the corresponding `ODR` (Output Data Register) bit selects **Pull-Down**:
```c
GPIOB->CRL &= ~(0x000000FF);
GPIOB->CRL |=  (0x00000088); /* PB0 and PB1 as Input with Pull-Up/Down */
GPIOB->ODR &= ~((1U << 0) | (1U << 1)); /* Select Pull-Down */
```

### 4. Direct Register LED Writing (`GPIOA->ODR` and `GPIOA->BSRR`)
To write an 8-bit pattern to `PA0`..`PA7` without disturbing other pins on Port A:
```c
GPIOA->ODR = (GPIOA->ODR & 0xFF00) | (pattern & 0x00FF);
```

### 5. Hardware 50Hz PWM Setup (`TIM4`)
For a $50\text{ Hz}$ frequency (period $T = 20\text{ ms} = 20,000\ \mu\text{s}$):
$$\text{Timer Clock} = \frac{f_{\text{SYS}}}{\text{PSC} + 1} = \frac{8\text{ MHz}}{7 + 1} = 1\text{ MHz} \quad (1\ \mu\text{s per tick})$$
$$\text{ARR} = 20,000 - 1 = 19,999 \implies \text{Period} = 20,000\ \mu\text{s} = 20\text{ ms} \ (50\text{ Hz})$$
$$\text{CCR3} = 10,000 \implies 50\%\ \text{Duty Cycle} \ (10\text{ ms HIGH}, 10\text{ ms LOW})$$

```c
TIM4->PSC = 7;
TIM4->ARR = 19999;
TIM4->CCR3 = 10000;
TIM4->CCMR2 |= (6U << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE; /* PWM Mode 1 */
TIM4->CCER  |= TIM_CCER_CC3E;                                  /* Enable Output */
TIM4->CR1   |= TIM_CR1_ARPE | TIM_CR1_CEN;                   /* Start Timer */
```

---

## 5. Deliverable Walkthrough

### Deliverable 1: Snake Effect
- **Phase 1 (Light one by one):**  
  `0b00000001` (1) $\to$ `0b00000011` (2) $\to$ `0b00000111` (3) $\to$ ... $\to$ `0b11111111` (8 LEDs ON).
- **Phase 2 (Switch off one by one):**  
  `0b11111110` $\to$ `0b11111100` $\to$ `0b11111000` $\to$ ... $\to$ `0b00000000` (All LEDs OFF).

### Deliverable 2: 50Hz PWM Signal
- **Hardware Pin (`PB8`):** High-precision $50.00\text{ Hz}$ square wave from TIM4.
- **Software Pin (`PA0`):** Precise $10\text{ ms}$ HIGH / $10\text{ ms}$ LOW toggle.
- **Breathing Effect:** Modulates duty cycle smoothly between $5\%$ and $95\%$.

### Deliverable 3: Controlled Counter (0 to 8)
- Counter values range strictly from $0$ to $8$ (9 total states).
- **Bar-Graph Style:**
  - $0$: All 8 LEDs OFF (`0x00`)
  - $1$: 1 LED ON (`0x01` — `PA0`)
  - $2$: 2 LEDs ON (`0x03` — `PA0`..`PA1`)
  - $3$: 3 LEDs ON (`0x07` — `PA0`..`PA2`)
  - ...
  - $8$: All 8 LEDs ON (`0xFF` — `PA0`..`PA7`)
  - Next press wraps back to $0$!

---

## 6. Build & Verification

The project is pre-configured and fully verified using the GNU ARM Embedded Toolchain:

```powershell
# From STM32/Task1/STM32F103C8TX/STM32F103C8TX/Debug:
make clean
make -j4 all
```

Output:
```
Finished building target: STM32F103C8TX.elf
arm-none-eabi-size STM32F103C8TX.elf 
   text    data     bss     dec     hex filename
   8688      20    1596   10304    2840 STM32F103C8TX.elf
Finished building: STM32F103C8TX.hex
```

- **Output Files:**
  - Binary ELF: `Core/Src/../Debug/STM32F103C8TX.elf`
  - Hex File: `Core/Src/../Debug/STM32F103C8TX.hex` (ready for ST-Link utility or Proteus simulation)
