# Appendix A — Source Coverage Checklist and Reconciliation Log

This appendix documents the systematic analysis, extraction, deduplication, and technical reconciliation performed across all seven source files provided for this session consolidation.

---

## A.1 Source File Coverage Checklist

**Table A.1 — Comprehensive source file coverage and extraction audit**

| Source File | Format & Scope | Unique Technical Content Identified | Where Incorporated in Reference |
|---|---|---|---|
| `Overview.md` | Markdown; Session overview & curriculum map | High-level 6-part session architecture, core philosophy ("no magic, no black boxes"), student learning outcomes. | Chapter 1 (1.2, 1.3), Chapter 10. |
| `STM32_Session_Instructor_Guide.docx` | DOCX; Instructor manual & pedagogical guide | 6-question instructor rule, 6-hour evening delivery schedule, 30-term CubeMX glossary, callout boxes for common student traps. | Chapter 1 (1.3.2), Chapter 8 (8.5), Chapter 11 (complete glossary). |
| `STM32_Session_Complete_Guide.md` | Markdown; Full training manual | Per-subsystem failure diagnostic checklists (GPIO, UART, I2C, SPI, ADC, Timers, DMA, RTOS), 7 hands-on labs with acceptance criteria, master formula table. | Chapter 8 (8.4 checklists, 8.5 labs, 8.6 formulas). |
| `STM32_Deep_Dive_In_Depth.md` | Markdown; Detailed textbook | 10-layer physical-to-firmware chain, ownership model, APB timer ×2 rule, 9-clock I²C unjamming, Cortex-M7 D-Cache coherency, 4-layer driver architecture, 20-question knowledge check. | Chapter 1 (1.4), Chapter 2 (2.1, 2.7), Chapter 4 (4.3.6), Chapter 5 (5.8), Chapter 6, Chapter 8 (8.7). |
| `STM32_Deep_Dive_Complete_Guide.md` | Markdown; Reference text | Code generation lifecycle, `.ioc` internals, USER CODE preservation, HAL vs LL vs Register comparison with PC13 examples, DMA modes, SPI modes table. | Chapter 2 (2.4, 2.8, 2.9), Chapter 3 (3.3, 3.6), Chapter 4 (4.2). |
| `STM32_Deep_Dive_Complete_Guide_Compact.md` | Markdown; Byte-identical to above | Redundant duplicate (verified byte-identical SHA256); consolidated without information loss. | Merged with Chapter 2–4. |
| `STM32_Deep_Dive_Complete_Guide.docx` | DOCX; Reference manual | System architecture diagrams, callout blocks, table formatting, retargeting printf implementation. | Chapter 4 (4.1.6), Chapter 9, diagrams 1–8. |

---

## A.2 Reconciled Contradictions and Technical Corrections

During the consolidation, several conflicting statements, outdated formulas, and family-specific inaccuracies were discovered in the source documents. These have been reconciled using official STMicroelectronics reference manuals, ARM architecture specifications, and NXP bus standards:

1. **ADC Trigger Timer on STM32F103**:
   - *Source Conflict*: An earlier script (`code.py`) hardcoded `TIM2_TRGO` as the external hardware trigger for `ADC1`.
   - *Correction*: On STM32F103 devices (Reference Manual RM0008, Section 11.3.6), the regular group external triggers for `ADC1` and `ADC2` are `TIM1_CC1`, `TIM1_CC2`, `TIM1_CC3`, `TIM2_CC2`, `TIM3_TRGO`, `TIM4_CC4`, and `EXTI11`. **`TIM2_TRGO` is NOT a valid trigger on STM32F103** (it is used on STM32F4). The example was corrected to use **`TIM3_TRGO`**, which is natively supported on all STM32F103 density lines.
2. **HAL Timebase Timer on STM32F103C8T6 with FreeRTOS**:
   - *Source Conflict*: Several tutorials generically recommend selecting `TIM6` or `TIM7` as the alternate timebase timer when FreeRTOS claims `SysTick`.
   - *Correction*: The STM32F103C8T6 (Medium-Density) contains **no basic timers** (`TIM6` and `TIM7` do not exist in silicon on this chip; only `TIM1`, `TIM2`, `TIM3`, and `TIM4` are populated). The documentation explicitly instructs students to allocate **`TIM4`** (or a spare general-purpose timer) on STM32F103 parts.
3. **I²C Rise-Time Mathematics and Constant**:
   - *Source Conflict*: Different files used $0.8 \times R_p C_b$ versus $0.8473 \times R_p C_b$ for rise-time calculations.
   - *Correction*: Under NXP I²C specification UM10204, rise time $t_r$ is measured from $30\%$ to $70\%$ of $V_{DD}$. The exact analytical solution is:
     $$t_r = R_p C_b \left( \ln\frac{1}{1 - 0.7} - \ln\frac{1}{1 - 0.3} \right) = R_p C_b (\ln 3.333 - \ln 1.428) \approx 0.8473 \times R_p C_b$$
     This rigorous constant was adopted. Furthermore, the minimum pull-up resistor was derived from $I_{OL} = 3\text{ mA}$ at $V_{OL} = 0.4\text{ V}$, yielding $R_{p,\min} \approx 967\ \Omega$ at $3.3\text{ V}$.
4. **I²C Sensor Burst Read Duration**:
   - *Source Conflict*: Sources claimed a 14-byte IMU burst read takes "~250 µs".
   - *Correction*: At 400 kHz Fast Mode, a 14-byte `HAL_I2C_Mem_Read` requires: START (1) + Address (8) + ACK (1) + Reg Addr (8) + ACK (1) + Repeated START (1) + Address+Read (8) + ACK (1) + 14 × [Data (8) + ACK (1)] + STOP (1) = 155 clock cycles.
     $$T = \frac{155}{400{,}000\text{ Hz}} = 387.5\ \mu\text{s}$$
     At 100 kHz Standard Mode, this consumes $1.55\text{ ms}$. This distinction is critical for students budgeting high-rate 1 kHz control loops.
5. **ADC Quantization Formula and Divisor ($2^N$ vs. $2^N - 1$)**:
   - *Source Conflict*: Documents inconsistently divided by $4096$ or $4095$ when calculating voltage from digital code.
   - *Correction*: The document clarifies both conventions: $4096$ ($2^N$) defines the mathematical width of one $\text{LSB}$ step interval ($\Delta V = \frac{V_{\text{REF}}}{2^N}$). However, because an unsigned 12-bit integer register saturates at `4095` (`0x0FFF`), software reconstructing full-scale voltage from digital code divides by $4095$ ($2^N - 1$) so that `Code == 4095` maps to exactly $3.300\text{ V}$.
6. **FreeRTOS SysCall Interrupt Priority Validation**:
   - *Source Conflict*: Some files claimed violating the priority rule causes an "immediate HardFault".
   - *Correction*: The ARM Cortex-M architecture itself does not trigger a hardware exception purely based on priority numbers. Rather, FreeRTOS executes an internal assertion `configASSERT(ucCurrentPriority >= ucMaxSysCallPriority)` inside `vPortValidateInterruptPriority()`. If assertions are enabled, it traps the core; if disabled, the unmasked ISR corrupts scheduler linked lists, producing memory faults later.
7. **Character Literal Escapes in Generated Code**:
   - *Source Conflict*: An earlier python generator script (`code.py`) emitted double-escaped `'\\n'` characters into C source blocks.
   - *Correction*: All C listings have been validated to contain clean, standard escape sequences (`'\n'`, `'\r'`, `'\0'`).

---

# Appendix B — Hardware-Specific Verification Items

The following architectural parameters must be verified against the exact silicon reference manual when porting this reference to other STM32 microcontrollers:

1. **GPIO Alternate Function Multiplexing**:
   - **STM32F1**: Uses legacy `AFIO` remap registers (`AFIO_MAPR`), allowing only fixed standard or remapped pin groupings.
   - **STM32F4/G4/H7**: Uses modern per-pin 4-bit multiplexer registers (`GPIOx_AFRL` and `GPIOx_AFRH`, supporting AF0 through AF15).
2. **DMA Request Routing**:
   - **STM32F1**: Fixed hardware channels. `ADC1` is hardwired to `DMA1 Channel 1`; `USART2_RX` is hardwired to `DMA1 Channel 6`.
   - **STM32F4**: Stream and Channel request matrix (e.g., Stream 0 Channel 0).
   - **STM32G4/L4/H7**: Dynamic request routing managed by the `DMAMUX` peripheral. Any peripheral can be mapped to any DMA channel.
3. **ADC External Hardware Triggers**:
   - Always verify the trigger table in the family Reference Manual (RM0008, RM0090, RM0440). Never assume trigger names are interchangeable across different chip families.
4. **Toolchain Version Parity**:
   - This document was verified against **STM32CubeIDE Version 2.2.0** and **STM32CubeMX Version 6.18.1**. Menu layouts, code generator options, and package directories may differ in legacy versions (CubeMX v4.x/v5.x).
