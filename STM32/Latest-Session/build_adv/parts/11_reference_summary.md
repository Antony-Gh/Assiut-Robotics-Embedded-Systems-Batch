# 11. Troubleshooting, Diagnostic Matrix, and Common Pitfalls

This chapter compiles a systematic diagnostic reference organizing common hardware, firmware, and architectural pitfalls encountered across the eight advanced topics.

**Table 11.1 — Master diagnostic matrix of advanced STM32 engineering pitfalls**

| Subsystem | Observed Symptom | Underlying Root Cause | Corrective Engineering Action |
|---|---|---|---|
| **Flash / EEPROM** | `HAL_FLASH_Program` returns `HAL_ERROR` | Flash memory block was not erased prior to writing (`0x00` already present) | Execute page or sector erase before programming. |
| **Flash / EEPROM** | HardFault or `PGERR` during write on F1 | Attempted 8-bit byte write or unaligned 32-bit write | STM32F1 Flash controller strictly enforces **16-bit HalfWord programming**! |
| **Flash / EEPROM** | Memory write succeeds but corrupts adjacent code | Storage address overlapped with compiled application code | Adjust linker script to reserve storage pages; place EEPROM bank in last Flash page. |
| **Encoders** | Motor spins forward but counter decrements | Encoder Channel A and B wiring inverted | Swap physical pin wires OR change timer input polarity in CubeMX to `Falling Edge`. |
| **Encoders** | Robot accumulates positioning error during high acceleration | Motor brush electrical noise inducing false edges on lines | Increase digital input glitch filter (`IC1F = 8` to `12`); route lines away from motor PWM. |
| **Encoders** | Speed calculation jumps to extreme negative values periodically | Unhandled 16-bit timer counter rollover ($65535 \rightarrow 0$) | Calculate delta using two's complement cast: `(int16_t)(curr - prev)`. |
| **Low-Power** | System runs 9× too slow after waking from Stop mode | Core defaulted to 8 MHz internal HSI clock; PLL remained OFF | **Mandatory**: Call `SystemClock_Config()` immediately following `HAL_PWR_EnterSTOPMode()`. |
| **Low-Power** | Current in Stop mode stuck at 2 mA instead of $20\ \mu\text{A}$ | Unused GPIO pins left floating as digital inputs (shoot-through) | Configure all unused pins in **Analog Mode (`GPIO_MODE_ANALOG`)**. |
| **Low-Power** | Microcontroller immediately wakes up from Sleep mode | SysTick 1 ms periodic interrupt waking the core | Call `HAL_SuspendTick()` before sleep; call `HAL_ResumeTick()` upon wake. |
| **Linker / Build** | `region 'RAM' overflowed by XXXX bytes` | Global arrays or memory buffers exceed available on-chip SRAM | Inspect `.map` file; mark static lookup tables with `const` to move them to Flash ROM. |
| **Linker / Build** | `undefined reference to '_sbrk'` | Calling `malloc()` / `printf()` without low-level syscalls | Implement minimal `_sbrk()` or disable dynamic memory allocation. |
| **Actuators** | H-Bridge driver heats up and smokes immediately | Shoot-through: High-side and low-side FETs turned ON simultaneously | Add hardware dead-time or use integrated driver ICs (TB6612FNG) with shoot-through lockout. |
| **Actuators** | Microcontroller resets randomly whenever motor starts | Motor inrush current sagging battery rail, causing MCU brownout | Separate digital and motor power rails; add $470\ \mu\text{F}$ bulk electrolytic capacitor at driver. |
| **Actuators** | Hobby servo twitches violently and fails to hold angle | PWM signal generated with software delays; frame period drifting | Generate PWM exclusively using a **hardware timer at 50 Hz** ($1.0 - 2.0\text{ ms}$ pulse width). |
| **Watchdogs** | Debugger disconnects every time a breakpoint is hit | Hardware IWDG continues counting while CPU is halted in GDB | Call `__HAL_DBGMCU_FREEZE_IWDG()` in `main()` during debug initialization. |
| **Watchdogs** | Microcontroller deadlocks, but watchdog NEVER resets it | Watchdog was refreshed inside a periodic timer interrupt | **Never refresh in an ISR!** Pet watchdog only in `main()` or via multi-task supervisor. |
| **Bootloaders** | HardFault occurs immediately upon jumping to application | Vector table not relocated; core used bootloader vectors | Update `SCB->VTOR = APPLICATION_ADDRESS` before jumping to application. |
| **Bootloaders** | Application crashes on first interrupt execution | Global interrupts were enabled during jump sequence | Call `__disable_irq()`, clear NVIC pending flags, and update MSP before branching. |
| **DAC** | Analog sine wave clipped flat at 0.2V and 3.1V | Internal op-amp output buffer enabled; cannot pull rail-to-rail | Scale digital LUT values within $[400, 3600]$ code range OR disable internal buffer. |

---

# 12. Master Comparison Tables

**Table 12.1 — Storage technologies comparison in embedded systems**

| Storage Medium | Technology | Erase Granularity | Write Endurance | Data Retention | Hardware Cost | Typical Robotics Usage |
|---|---|---|---|---|---|---|
| **On-Chip SRAM** | Volatile (CMOS) | Byte / Word | Infinite | None (Lost at power-down) | Zero extra cost | Runtime stack, heap, DMA buffers. |
| **On-Chip NOR Flash** | Non-Volatile (Floating Gate) | **Page (1 KB)** or **Sector (16–128 KB)** | **$10{,}000\text{ cycles}$** | $> 20\text{ years}$ | Zero extra cost | Program firmware, calibration parameters. |
| **External I²C EEPROM** | Non-Volatile (Byte-alterable) | Single Byte | $1{,}000{,}000\text{ cycles}$ | $> 100\text{ years}$ | Low ($0.30) | Frequently updated configuration logs. |
| **External SPI FRAM** | Non-Volatile (Ferroelectric) | Single Byte | $10^{14}\text{ cycles}$ (Near infinite) | $> 10\text{ years}$ | Moderate ($2.00) | High-speed crash telemetry flight data recorders. |

---

**Table 12.2 — DC Motor drivers comparison**

| Parameter | L298N | TB6612FNG | DRV8833 |
|---|---|---|---|
| **Transistor Technology** | Bipolar (BJT) | Dual Power MOSFET | Dual Power MOSFET |
| **Internal Voltage Drop** | High ($1.8\text{ V} - 3.2\text{ V}$) | Very Low ($0.2\text{ V} - 0.4\text{ V}$) | Very Low ($0.15\text{ V} - 0.3\text{ V}$) |
| **Operating Motor Voltage** | $4.5\text{ V} - 46\text{ V}$ | $4.5\text{ V} - 15\text{ V}$ | $2.7\text{ V} - 10.8\text{ V}$ |
| **Continuous Current / Ch** | $2.0\text{ A}$ (With bulky heatsink) | $1.2\text{ A}$ (Runs cool) | $1.5\text{ A}$ (Runs cool) |
| **Logic Supply Voltage** | $5.0\text{ V}$ | $2.7\text{ V} - 5.5\text{ V}$ (Native 3.3V) | $2.7\text{ V} - 5.5\text{ V}$ (Native 3.3V) |
| **Recommended Verdict** | Obsolete; avoid in new designs | **Universal standard for 12V robotics** | **Ideal for 1S/2S battery robotics** |

---

# 13. Advanced Technical Glossary

- **ART Accelerator (Adaptive Real-Time Memory Accelerator)**: STMicroelectronics' proprietary hardware memory accelerator on Cortex-M4/M7 cores integrating instruction prefetch buffers and branch caches, achieving $0\text{-wait-state}$ execution at $168\text{ MHz}$.
- **Back-EMF (Counter-Electromotive Force)**: An electrical voltage generated in motor windings opposing the applied voltage, proportional to rotational speed.
- **Bootloader**: A specialized firmware program residing at Flash base executing upon reset to download, verify, and install application binaries.
- **CCMRAM (Core Coupled Memory RAM)**: A dedicated 64 KB SRAM block on STM32F405 connected directly to the Cortex-M4 D-bus, providing 0-wait-state access; inaccessible by DMA.
- **CPR (Counts Per Revolution)**: Total decoded digital position increments registered by the microcontroller per complete shaft rotation ($4 \times \text{PPR}$ in x4 mode).
- **Dead-Time**: A deliberate hardware delay inserted between turning off one MOSFET and turning on the complementary MOSFET in an H-Bridge to prevent shoot-through.
- **DFU (Device Firmware Upgrade)**: An official USB standard protocol supported by STM32 factory bootloaders allowing firmware updates over a USB cable.
- **Duty Cycling**: An engineering technique alternating between brief active execution bursts and deep low-power sleep states to minimize average current consumption.
- **Early Wakeup Interrupt (EWI)**: An emergency interrupt generated by the WWDG one tick before hardware reset, allowing failsafe logging.
- **Flyback Diode**: A fast-recovery diode placed anti-parallel across an inductive load to safely recirculate inductive flyback current when switched off.
- **H-Bridge**: An electronic circuit topology featuring four switches capable of applying voltage in either direction across a DC motor.
- **Inductive Kickback**: High-voltage inductive spike ($V = -L \frac{di}{dt}$) generated across a coil when electrical current is abruptly interrupted.
- **ISP (In-System Programming)**: Flashing or updating microcontroller firmware while soldered into its final target printed circuit board.
- **IWDG (Independent Watchdog)**: A safety countdown timer clocked by an independent $40\text{ kHz}$ internal RC oscillator (LSI), surviving main clock failure.
- **Linker Script (`*.ld`)**: An architectural configuration script directing the GNU linker where to locate code, constants, stack, heap, and variables in physical memory.
- **LMA (Load Memory Address)**: The physical address where a memory segment (such as initialized `.data`) is stored in non-volatile Flash ROM.
- **Microstepping**: A stepper driver technique modulating sinusoidal coil currents to position the rotor at fractional step increments ($1/2$ to $1/32$), eliminating resonance.
- **Optocoupler**: An electrical isolation component transferring digital signals via optical light beams, isolating high-power motor rails from low-voltage logic.
- **PPR (Pulses Per Revolution)**: The number of electrical square-wave cycles generated on a single encoder channel per physical revolution.
- **Quadrature**: An electrical phase relationship where two square-wave signals are offset by exactly $90^\circ$ ($\frac{\pi}{2}$ radians).
- **Settling Time**: The time required for a DAC analog output to stabilize to within $\pm 0.5\text{ LSB}$ of target voltage following a code step.
- **Shoot-Through**: A catastrophic failure mode where both high-side and low-side transistors in an H-Bridge turn ON simultaneously, shorting the battery rail directly to GND.
- **Star Grounding**: A circuit layout methodology connecting high-power motor ground and low-power digital logic ground at a single physical point at the battery terminal.
- **VMA (Virtual Memory Address)**: The runtime physical address where a program segment resides and executes in memory (e.g., SRAM).
- **VTOR (Vector Table Offset Register)**: System Control Block register (`SCB->VTOR`) pointing to the base memory address of the active Interrupt Vector Table.
- **Wear-Leveling**: An algorithmic storage strategy distributing Flash erase cycles evenly across memory blocks to prevent premature gate degradation.
- **WWDG (Window Watchdog)**: A safety timer clocked from the APB1 bus enforcing that refreshes must occur within a strict time window, catching runaway fast loops.

---

# 14. Master Formula and Calculation Reference

**Table 14.1 — Master engineering formula reference**

| Parameter | Governing Mathematical Formula | Key Variables & Physical Units |
|---|---|---|
| **Dynamic Power** | $P_{\text{dynamic}} = C_{\text{load}} \times V_{DD}^2 \times f_{\text{SYSCLK}}$ | $C$: Capacitance (F), $V$: Supply (V), $f$: Clock frequency (Hz) |
| **Encoder CPR** | $\text{CPR} = 4 \times \text{PPR}$ | In `Encoder Mode TI1 and TI2` ($4\times$ edge counting) |
| **Shaft RPM** | $\text{RPM} = \frac{\Delta \text{Count}}{\text{CPR}} \times \frac{60}{\Delta t}$ | $\Delta \text{Count}$: Signed ticks, $\Delta t$: Sample period (s) |
| **Linear Velocity** | $v = \frac{\pi \times D \times \Delta \text{Count}}{\text{CPR} \times \Delta t}$ | $D$: Wheel diameter (m), $v$: Linear speed (m/s) |
| **Traveled Distance**| $s = \frac{\text{Total Count}}{\text{CPR}} \times (\pi \times D)$ | $s$: Total distance in meters |
| **Duty Cycle Current**| $I_{\text{avg}} = D \times I_{\text{active}} + (1 - D) \times I_{\text{sleep}}$ | $D = \frac{T_{\text{active}}}{T_{\text{active}} + T_{\text{sleep}}}$, Current in mA or $\mu\text{A}$ |
| **Battery Lifetime**| $\text{Hours} = \frac{C_{\text{batt}} \times \eta}{I_{\text{avg}}}$ | $C_{\text{batt}}$: Capacity (mAh), $\eta$: Efficiency factor ($\approx 0.85$) |
| **Motor PWM Freq** | $f_{\text{PWM}} = \frac{f_{\text{TIM}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}$ | $f_{\text{PWM}} = 20{,}000\text{ Hz}$ standard ultrasonic frequency |
| **Servo Pulse Width**| $\text{CCR} = 1000 + \left(\text{Angle}^\circ \times \frac{1000}{180}\right)$ | Timer tick $= 1.0\ \mu\text{s}$, $50\text{ Hz}$ PWM frame ($\text{ARR} = 19999$) |
| **Inductive Spike** | $V_L = -L \frac{di}{dt}$ | $L$: Coil inductance (H), $\frac{di}{dt}$: Current slew rate (A/s) |
| **IWDG Timeout** | $T_{\text{IWDG}} = \frac{P \times (\text{RLR} + 1)}{f_{\text{LSI}}}$ | $P \in \{4, 8, 16, 32, 64, 128, 256\}$, $f_{\text{LSI}} \approx 40\text{ kHz}$ |
| **DAC Output Volt** | $V_{\text{OUT}} = V_{\text{REF}+} \times \frac{\text{DOR}}{4095}$ | $\text{DOR} \in [0, 4095]$ 12-bit integer code |
| **DAC Waveform Freq**| $f_{\text{wave}} = \frac{f_{\text{TIM}}}{(\text{PSC} + 1) \times (\text{ARR} + 1) \times N}$ | $N$: Points in lookup table (e.g. 128) |
| **PWM DAC Cutoff** | $f_c = \frac{1}{2\pi R C}$ | 2nd-order analog RC low-pass filter cutoff frequency |

---

# 15. References and Official Documentation

1. **STMicroelectronics RM0008 Reference Manual**: *STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx advanced ARM-based 32-bit MCUs*. Rev 21.
2. **STMicroelectronics RM0090 Reference Manual**: *STM32F405/415, STM32F407/417, STM32F427/437 and STM32F429/439 advanced ARM-based 32-bit MCUs*. Rev 19.
3. **STMicroelectronics PM0056 Programming Manual**: *STM32F10xxx/20xxx/21xxx/L1xxxx Cortex-M3 programming manual*.
4. **STMicroelectronics PM0214 Programming Manual**: *STM32 Cortex-M4 MCUs and MPUs programming manual*.
5. **STMicroelectronics AN3155 Application Note**: *USART protocol used in the STM32 bootloader*.
6. **STMicroelectronics AN3156 Application Note**: *USB DFU protocol used in the STM32 bootloader*.
7. **STMicroelectronics AN2586 Application Note**: *STM32F10xxx hardware development: getting started*.
8. **STMicroelectronics AN4013 Application Note**: *STM32 cross-series timer overview*.
9. **STMicroelectronics AN3960 Application Note**: *EEPROM emulation in STM32F40x/STM32F41x microcontrollers*.
10. **ARM DDI 0403E.e**: *ARMv7-M Architecture Reference Manual*. ARM Limited.
11. **Toshiba Semiconductor**: *TB6612FNG Dual DC Motor Driver Datasheet*. Rev 2.0.
12. **Texas Instruments**: *DRV8833 Dual H-Bridge Motor Driver Datasheet*. SLVSAR1E.
13. **Allegro MicroSystems**: *A4988 DMOS Microstepping Driver with Translator Datasheet*.

---

# 16. Final Recommended Learning Roadmap

To progress from intermediate embedded programming to professional robotics firmware engineering, master topics in the following sequence:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ Phase 5: Autonomous Robotics & Mission-Critical Architecture│
 │ Custom bootloaders, FreeRTOS supervisor, Fail-safe storage  │
 ├─────────────────────────────────────────────────────────────┤
 │ Phase 4: Motion Control & Actuation                         │
 │ Quadrature encoders, PID velocity loops, 20 kHz PWM H-Bridge│
 ├─────────────────────────────────────────────────────────────┤
 │ Phase 3: Power Optimization & Memory Hygiene                │
 │ Low-power Stop duty-cycling, Flash wear-leveling, Linker .ld│
 ├─────────────────────────────────────────────────────────────┤
 │ Phase 2: Signal Synthesis & Sensor Fusion                   │
 │ DAC DMA waveform generation, IMU calibration persistence    │
 ├─────────────────────────────────────────────────────────────┤
 │ Phase 1: Core Peripheral Mastery (Baseline Reference)       │
 │ GPIO, Clocks, Timers, Interrupts, DMA, UART, SPI, I2C, ADC  │
 └─────────────────────────────────────────────────────────────┘
```
