# 9. Topic 8 — Digital-to-Analog Converter (DAC) and Waveform Generation

While the Analog-to-Digital Converter (ADC, detailed in Chapter 5 of the baseline reference) enables microcontrollers to sample continuous real-world signals into discrete numbers, the reverse operation is equally vital in advanced embedded systems.

A **Digital-to-Analog Converter (DAC)** transforms digital numeric codes computed in software back into continuous electrical voltages ($0\text{ V} \text{ to } V_{\text{REF}+}$).

Embedded applications requiring true analog outputs include:
- Generating dynamic analog reference setpoints for motor current-chopper controllers.
- Synthesizing pure sinusoidal acoustic audio tones and alert chimes.
- Arbitrary function generation for scientific instrumentation and sensor stimulation.
- Driving proportional hydraulic valves and analog piezo positioners.

This chapter details the internal architecture and settling dynamics of STM32 DAC peripherals, develops the high-performance **Timer TRGO $\rightarrow$ Circular DMA $\rightarrow$ DAC** waveform synthesis pipeline on the **STM32F405RGT6**, and provides verified hardware alternatives for the **STM32F103C8T6** (which lacks an internal DAC).

![Figure 8.1 — Digital-to-analog conversion pipeline, circular DMA streaming, and PWM filter alternative](diagrams_adv/fig8_dac_and_waveform.png)
*Figure 8.1 — (A) High-speed hardware DAC waveform generation pipeline on STM32F405 using Timer TRGO, Circular DMA, and 12-bit DAC1; (B) STM32F103 low-pass filtered PWM alternative topology with 2nd-order RC circuit.*

---

## 9.1 DAC Fundamentals: Converting Bits to Volts

An ideal Digital-to-Analog Converter maps an $N$-bit digital integer code ($\text{DOR}$) into an analog voltage ($V_{\text{OUT}}$) linearly scaled against an electrical reference voltage ($V_{\text{REF}+}$):

$$V_{\text{OUT}} = V_{\text{REF}+} \times \frac{\text{DOR}}{2^N - 1}$$

For a 12-bit DAC ($N = 12 \implies 2^{12} - 1 = 4095$):

$$V_{\text{OUT}} = V_{\text{REF}+} \times \frac{\text{DOR}}{4095}$$

Where $V_{\text{REF}+}$ is typically tied to the analog supply rail $V_{DDA}$ ($3.30\text{ V}$).
The voltage weight of one Least Significant Bit ($\text{LSB}$) is:

$$\text{LSB} = \frac{V_{\text{REF}+}}{4095} = \frac{3.30\text{ V}}{4095} \approx 0.8058\text{ mV}$$

---

### 9.1.1 Settling Time, Glitch Energy, and Output Buffering

Real-world DAC performance is defined by two key physical parameters:
1. **Settling Time ($T_{\text{settling}}$)**: The time required for the analog output voltage to transition and settle to within $\pm 0.5\text{ LSB}$ of the target value following a full-scale digital code step.
   - On the STM32F405 DAC, typical settling time is **$3.0\ \mu\text{s} \text{ to } 6.0\ \mu\text{s}$**, limiting maximum clean analog output bandwidth to $\sim 150\text{ kHz} - 300\text{ kHz}$.
2. **The Internal Output Buffer (`DAC_OUTPUTBUFFER`)**:
   The STM32 DAC includes an internal operational amplifier configured as a voltage follower buffer:

**Table 9.1 — Output buffer trade-offs in STM32 DAC**

| Mode | Output Impedance | Drive Capability | Voltage Output Range | Best Engineering Application |
|---|---|---|---|---|
| **Buffer ENABLED** | Low ($< 1\ \Omega$) | Can drive resistive loads down to **$5\text{ k}\Omega$** and capacitive loads up to **$50\text{ pF}$** directly. | **Restricted Rail**: $0.2\text{ V} \le V_{\text{OUT}} \le V_{DDA} - 0.2\text{ V}$ (Op-amp cannot swing to true rail) | Driving external amplifiers, headphone stages, and analog inputs directly. |
| **Buffer DISABLED** | **High ($\sim 15\text{ k}\Omega$)** (Unbuffered internal resistor ladder) | **Extremely weak**; even a $100\text{ k}\Omega$ load causes massive voltage drop! | **True Rail-to-Rail**: True $0.0\text{ V}$ up to full $V_{DDA}$. | High-impedance CMOS inputs or when followed by an external precision op-amp. |

---

## 9.2 The STM32F405 Hardware DAC Peripheral

The **STM32F405RGT6** features **two independent 12-bit DAC channels**:
- **DAC Channel 1**: Hardwired to pin **`PA4`**.
- **DAC Channel 2**: Hardwired to pin **`PA5`**.

Both channels can operate independently or be updated simultaneously in dual-DAC mode.

### 9.2.1 Setting Static Analog Voltages in HAL

**Listing 9.1 — Configuring and generating static analog voltages on STM32F405**

```c
#include "main.h"

extern DAC_HandleTypeDef hdac;

void App_DAC_Init(void)
{
    /* Start DAC Channel 1 (Pin PA4) */
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}

/**
 * @brief Sets output voltage in millivolts (0 to 3300 mV).
 * @param voltage_mv Target voltage in mV.
 */
void App_DAC_SetMillivolts(uint32_t voltage_mv)
{
    if (voltage_mv > 3300) voltage_mv = 3300;

    /* Code = (V_target / 3300) * 4095 */
    uint32_t dac_code = (voltage_mv * 4095UL) / 3300UL;

    /* Write 12-bit right-aligned value */
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_code);
}
```

---

## 9.3 High-Speed Waveform Synthesis: Timer TRGO $\rightarrow$ DMA $\rightarrow$ DAC

Attempting to synthesize continuous waveforms (such as a $1\text{ kHz}$ sine wave) by updating `HAL_DAC_SetValue()` in a software loop wastes massive CPU cycles and induces phase jitter.

The professional architectural solution is the **Hardware Timer $\rightarrow$ DMA $\rightarrow$ DAC Pipeline**:

```text
 [TIM6 Basic Timer] ──► Generates TRGO Trigger at rate F_sample
                               │
                               ▼
 [DMA1 Stream 5] ────► Fetches next 16-bit word from SRAM Sine Table
                               │
                               ▼
 [DAC1 Peripheral] ──► Latches word into DAC_DHR12R1 ──► [Continuous Analog Output PA4]
```

1. **Precomputed Lookup Table (LUT)**: An array of $N$ discrete waveform points is stored in memory.
2. **Timer TRGO**: A basic hardware timer (`TIM6`) triggers periodic updates with zero jitter.
3. **Circular DMA**: The DMA controller automatically transfers the next point from memory to the DAC holding register upon every timer tick, cycling continuously in a closed loop.
4. **Result**: A continuous, mathematically pure analog waveform generated with **$0\%$ CPU utilization**!

---

### 9.3.1 Mathematical Frequency Derivation
To generate an analog waveform of desired frequency $f_{\text{wave}}$ using a lookup table of $N$ points:

$$f_{\text{sample}} = N \times f_{\text{wave}}$$

The timer update frequency ($f_{\text{timer}}$) must equal $f_{\text{sample}}$:

$$f_{\text{sample}} = \frac{f_{\text{TIM}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}$$

Equating and solving for the timer period:

$$(\text{PSC} + 1) \times (\text{ARR} + 1) = \frac{f_{\text{TIM}}}{N \times f_{\text{wave}}}$$

#### Worked Calculation: $1000\text{ Hz}$ Sine Wave on STM32F405
- Target Waveform: $f_{\text{wave}} = 1000\text{ Hz}$.
- Lookup Table Size: $N = 128\text{ points}$.
- Required Trigger Rate: $f_{\text{sample}} = 128 \times 1000 = 128{,}000\text{ Hz}$.
- On STM32F405, `TIM6` is clocked from APB1 ($f_{\text{TIM}} = 84\text{ MHz}$).
1. Set $\text{PSC} = 0$ (no clock prescaling):
   $$\text{ARR} + 1 = \frac{84{,}000{,}000\text{ Hz}}{128{,}000\text{ Hz}} = 656.25 \implies \mathbf{\text{ARR} = 655}$$
2. Actual frequency:
   $$f_{\text{actual}} = \frac{84{,}000{,}000}{(1) \times (656) \times 128} \approx \mathbf{1000.38\text{ Hz}}\quad (\text{Error} < 0.04\%)$$

---

### 9.3.2 Production DMA Sine Wave Generator Code

**Listing 9.2 — Zero-CPU continuous sine wave synthesis on STM32F405**

```c
#include "main.h"
#include <math.h>

extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim6;

#define SINE_POINTS   128

/* Lookup table precomputed in memory (Centered at 1.65V, peak-to-peak 2.6V) */
static uint16_t Sine_LUT[SINE_POINTS];

void App_InitSineTable(void)
{
    for (int i = 0; i < SINE_POINTS; i++) {
        /* Sine angle in radians from 0 to 2*pi */
        float rad = (2.0f * 3.14159265f * (float)i) / (float)SINE_POINTS;
        
        /* Map sin [-1.0, +1.0] to DAC code range [400, 3600] to stay inside buffered linear region */
        float val = 2048.0f + (1600.0f * sinf(rad));
        Sine_LUT[i] = (uint16_t)val;
    }
}

void App_StartWaveform(void)
{
    App_InitSineTable();

    /* 1. Start DAC1 in Circular DMA mode */
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)Sine_LUT, SINE_POINTS, DAC_ALIGN_12B_R);

    /* 2. Start Hardware Timer TIM6 to generate TRGO triggers */
    HAL_TIM_Base_Start(&htim6);
    
    /* CPU is now 100% free; waveform streams autonomously in silicon! */
}
```

---

## 9.4 STM32 Compatibility & Alternatives for the STM32F103C8T6

> [!IMPORTANT]
> **Crucial Hardware Limitation**: The **STM32F103C8T6 contains NO internal DAC peripheral**.
> In the STM32F1 family, internal 12-bit DACs are populated **only on High-Density and Connectivity lines** (parts with $\ge 256\text{ KB}$ Flash, such as the STM32F103RC, STM32F103RD, and STM32F103RE).

When developing on the STM32F103C8T6, students have two proven engineering alternatives:

### Alternative 1: External I²C or SPI DAC IC (Recommended for Precision)
Connect an external monolithic DAC chip:
- **Microchip MCP4725**: Low-cost 12-bit I²C DAC with on-chip EEPROM. Capable of updating at up to $100\text{ kSPS}$ over $400\text{ kHz}$ Fast-Mode I²C.
- **Microchip MCP4921**: High-speed 12-bit SPI DAC. With a $20\text{ MHz}$ SPI clock, it achieves $> 1.0\text{ MSPS}$ update rates with sub-microsecond settling time.

---

### Alternative 2: High-Frequency Timer PWM with a 2nd-Order Analog RC Filter
For cost-sensitive applications where only an approximate analog DC voltage or slow audio tone is needed, a **PWM DAC** can be synthesized using a timer and an analog low-pass filter:

```text
 PWM Output (PA8, 100 kHz) ───[ R1: 10 kΩ ]───┬───[ R2: 10 kΩ ]───┬───► Filtered Analog Out
                                              │                   │
                                          [ C1: 100 nF ]      [ C2: 100 nF ]
                                              │                   │
                                             GND                 GND
```

#### The Physics of the PWM DAC
The time-averaged DC voltage of a PWM square wave is directly proportional to its duty cycle:

$$V_{\text{DC}} = V_{DD} \times \text{Duty Cycle} = 3.3\text{ V} \times \frac{\text{CCR}}{\text{ARR} + 1}$$

A 2nd-order analog RC low-pass filter attenuates the high-frequency $100\text{ kHz}$ switching harmonics by over $-80\text{ dB}$, leaving a smooth analog voltage.
- **Cutoff Frequency ($f_c$)**:
  $$f_c = \frac{1}{2\pi R C} = \frac{1}{2\pi \times 10{,}000 \times 100 \times 10^{-9}} \approx 159\text{ Hz}$$
- *Trade-off*: The RC filter introduces a phase settling delay ($\sim 5\text{ ms}$). It is ideal for programmable analog reference thresholds and DC biases, but cannot synthesize high-frequency signals.
