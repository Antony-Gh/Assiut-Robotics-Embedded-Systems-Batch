# 5. Part 4 — Sensors, ADC, and Real-Time Data Acquisition

Physical engineering systems do not deal in abstract software states; they interact with the physical world through voltages, currents, temperatures, pressures, accelerations, and electromagnetic fields. For an embedded microcontroller to perceive its environment, it must interface with two primary classes of sensors:
1. **Analog Sensors**: Transduce physical phenomena into continuous electrical potentials ($0\text{ V}$ to $V_{DD}$), which the microcontroller quantizes using an **Analog-to-Digital Converter (ADC)**.
2. **Digital Sensors**: Contain integrated front-end instrumentation amplifiers, ADCs, and digital logic, exposing internal registers accessed over serial buses (**I²C**, **SPI**, or **UART**).

This chapter examines the physical, mathematical, and algorithmic mechanisms of embedded data acquisition. We analyze ADC quantization mechanics, input impedance and sampling capacitor dynamics, internal STM32 calibration channels, register-level digital sensor interrogation, deterministic real-time data flows, digital filtering (moving average and fixed-point exponential smoothing), and high-performance DMA cache coherency on ARM Cortex-M7 architectures.

![Figure 5.1 — Analog signal conditioning chain, ADC SAR architecture, and quantization transfer function](diagrams/fig8_adc_chain_quantization.png)
*Figure 5.1 — Analog data acquisition: (A) Complete front-end signal chain from physical transducer to DMA buffer; (B) Internal Successive Approximation Register (SAR) ADC architecture with sampling capacitor $C_{\text{ADC}}$ and comparator; (C) Quantization staircase showing finite resolution steps, quantization error ($\pm 0.5\text{ LSB}$), and full-scale saturation.*

---

## 5.1 The Sensor Taxonomy

Before selecting peripheral hardware and writing driver code, an engineer must classify the sensor according to its electrical interface and acquisition timing requirements.

**Table 5.1 — Embedded sensor interface taxonomy**

| Category | Transducer Examples | Physical Signal | STM32 Interfacing Hardware | Acquisition Strategy |
|---|---|---|---|---|
| **Analog Voltage** | Potentiometer, LM35, Force-Sensing Resistor (FSR), Sharp IR | Continuous $0\text{ V} - V_{\text{REF}}$ | ADC (Internal SAR) | Timer-triggered DMA or Polled ADC |
| **Variable Resistance** | NTC/PTC Thermistor, Photoresistor (LDR), Strain Gauge | Variable resistance ($R_x$) | Voltage divider / Wheatstone Bridge → ADC | Periodic ADC conversion + math conversion |
| **Digital Register Bus** | MPU6050 (IMU), BMP280 (Pressure), BNO055 (AHRS) | Byte packets over synchronous bus | I²C or SPI Peripheral | Periodic polled/DMA burst read + DRDY interrupt |
| **Serial ASCII / Stream** | NMEA GPS (NEO-6M), LIDAR (TF-Luna), Ultrasonic | Asynchronous serial byte stream | UART / USART | Circular DMA with IDLE Line detection |
| **Pulse / Frequency** | Optical Wheel Encoder, Flow meter, Hall Effect | Digital pulses, square wave frequency | Timer Input Capture / Encoder Interface | Hardware counter gating (Zero CPU load) |
| **PWM Output** | Ultrasonic (HC-SR04), MEMS Inclinometers | High-time pulse width ($T_{\text{high}}$) | Timer Input Capture (Dual channel) | Timer capture on rising and falling edges |

---

## 5.2 Analog-to-Digital Converter (ADC) Fundamentals

The STM32 internal ADC is a **Successive Approximation Register (SAR)** converter. A SAR ADC converts an unknown continuous analog input voltage $V_{\text{IN}}$ into an $N$-bit digital integer code by performing a binary search across $N$ comparison cycles using an internal capacitive Digital-to-Analog Converter (DAC) and high-speed analog comparator.

### 5.2.1 Resolution, Reference Voltage ($V_{\text{REF}}$), and Quantization

- **Resolution ($N$)**: The number of binary bits produced per conversion (typically 12 bits on STM32F1/F4, up to 16 bits on STM32H7). An $N$-bit converter produces $2^N$ discrete digital output codes (e.g., $2^{12} = 4096$ levels, spanning codes `0` to `4095`).
- **Reference Voltage ($V_{\text{REF}+}$ / $V_{\text{REF}-}$)**: The upper and lower electrical boundary voltages defining the converter's full-scale input span. On smaller packages (e.g., LQFP48 on STM32F103 "Blue Pill"), $V_{\text{REF}+}$ is internally tied directly to the analog supply rail $V_{DDA}$ ($3.3\text{ V}$), and $V_{\text{REF}-}$ is tied to $V_{SSA}$ ($0\text{ V}$). On larger packages (LQFP100+), dedicated $V_{\text{REF}+}$ pins allow connecting precision low-noise voltage references.

#### Least Significant Bit (LSB) Voltage
The voltage weight of one Least Significant Bit ($\text{LSB}$) represents the fundamental quantization step size:

$$\text{LSB} = \frac{V_{\text{REF}+} - V_{\text{REF}-}}{2^N} = \frac{3.3\text{ V}}{4096} \approx 0.80566\text{ mV}$$

#### Ideal Conversion Transfer Functions
For an ideal SAR ADC with floor-type integer truncation:

$$\text{Code} = \left\lfloor \frac{V_{\text{IN}}}{V_{\text{REF}+}} \times 2^N \right\rfloor$$

To reconstruct the equivalent analog input voltage from a digital reading in application code:

$$V_{\text{IN}} = \text{Code} \times \frac{V_{\text{REF}+}}{2^N - 1} = \text{Code} \times \frac{3.3\text{ V}}{4095}$$

> [!NOTE]
> **Divisor Convention ($2^N$ vs. $2^N - 1$)**: Pure theoretical quantization models divide by $2^N$ ($4096$). However, because the maximum digital code that can fit in a 12-bit register is strictly $4095$ (`0x0FFF`), scaling by $\frac{V_{\text{REF}}}{4095}$ ensures that an input voltage exactly equal to $V_{\text{REF}}$ produces exactly $3.300\text{ V}$ in software. For high-precision metrology, use the calibrated internal reference voltage described in Section 5.3.

#### Resolution vs. Accuracy vs. ENOB
A common junior engineering mistake is confusing **resolution** with **accuracy**:
- **Resolution**: The finest step size the converter can distinguish ($12\text{ bits} \implies 0.8\text{ mV}$).
- **Accuracy**: The real-world deviation between the converted digital code and the true absolute voltage, dictated by **Offset Error**, **Gain Error**, **Integral Non-Linearity (INL)**, and **Differential Non-Linearity (DNL)**.
- **Effective Number of Bits (ENOB)**: Reflects total dynamic performance including thermal noise and harmonic distortion ($THD$):
  $$\text{ENOB} = \frac{\text{SINAD} - 1.76\text{ dB}}{6.02}$$
  A typical STM32 12-bit ADC has an ENOB between **$10.2$ and $10.8$ bits** when sampled over a noisy breadboard.

---

### 5.2.2 The Sampling Switch, $R_{\text{AIN}}$, and Settling Dynamics

To perform a conversion, the SAR ADC must first capture an accurate electrical charge from the external signal onto its internal sampling capacitor $C_{\text{ADC}}$ (typically $5\text{ pF}$ to $8\text{ pF}$ on STM32) through the internal analog switch resistance $R_{\text{ADC}}$ (typically $1\text{ k}\Omega$).

```text
External Circuit           STM32 Pin Boundary       Internal ADC Core
   ┌──────────┐                                     Sampling Switch
───┤   RAIN   ├───────────────┬───────────────────────────/ ─────┬───────► To SAR
   └──────────┘               │                                  │        Comparator
   Source Impedance       ┌───┴───┐                          ┌───┴───┐
                          │ CPIN  │ (Trace/Pin ~5pF)         │ CADC  │ (~8pF)
                          └───┬───┘                          └───┬───┘
   GND ───────────────────────┴──────────────────────────────────┴─────── GND
```

When the sampling switch closes for duration $T_{\text{sample}}$, an RC low-pass circuit is formed with time constant:

$$\tau = (R_{\text{AIN}} + R_{\text{ADC}}) \times C_{\text{ADC}}$$

For a 12-bit ADC, the voltage across $C_{\text{ADC}}$ must settle to within $\frac{1}{4}\text{ LSB}$ ($99.988\%$ of the true input voltage) before the switch opens and the conversion begins. Achieving this requires a minimum settling time of at least **$8.3$ time constants**:

$$T_{\text{sample}} \ge 8.3 \times (R_{\text{AIN}} + R_{\text{ADC}}) \times C_{\text{ADC}}$$

#### Practical Engineering Impact
If the external source impedance $R_{\text{AIN}}$ is high (for example, a high-resistance $100\text{ k}\Omega$ voltage divider or unbuffered potentiometer) and $T_{\text{sample}}$ is set too short (e.g., $1.5$ ADC clock cycles in CubeMX):
1. The sampling capacitor does not finish charging.
2. The ADC converts an incorrect, lower voltage.
3. Charge injection from switching between channels bleeds over, causing **cross-channel crosstalk** (reading channel 1 corrupts channel 2).

> [!WARNING]
> **Maximum Source Impedance Rule**: According to STM32 datasheets, for the fastest sampling time ($1.5$ cycles at $14\text{ MHz}$), $R_{\text{AIN}}$ must not exceed **$0.4\text{ k}\Omega$**. If your sensor or resistive divider has an output impedance of $10\text{ k}\Omega$ to $50\text{ k}\Omega$, you **must** either:
> 1. Increase the sampling time in CubeMX to **$71.5$ or $239.5$ cycles**, or
> 2. Insert an external operational amplifier configured as an **analog voltage follower (buffer)**.

---

### 5.2.3 Conversion Time and ADC Clock Limits

Total ADC conversion time per channel is the sum of sampling time and successive approximation cycles:

$$T_{\text{conv}} = T_{\text{sample}} + 12.5\text{ ADC clock cycles (for 12-bit resolution)}$$

On the STM32F103 family:
- **Maximum ADC Clock Frequency ($f_{\text{ADC}}$)**: Strictly **$14\text{ MHz}$**. Exceeding $14\text{ MHz}$ results in non-monotonicity and missing codes.
- Since SYSCLK is typically $72\text{ MHz}$ and APB2 ($f_{\text{PCLK2}}$) is $72\text{ MHz}$, the ADC Prescaler in CubeMX must be set to at least **$\text{PCLK2 divided by 6}$** ($72\text{ MHz} / 6 = 12\text{ MHz}$) or **$\text{divided by 8}$** ($9\text{ MHz}$).

#### Worked Example: Maximum Sampling Rate on STM32F103
With $f_{\text{ADC}} = 12\text{ MHz}$ ($T_{\text{ADCCLK}} = \frac{1}{12\text{ MHz}} \approx 83.33\text{ ns}$) and setting $T_{\text{sample}} = 1.5\text{ cycles}$:

$$T_{\text{conv}} = 1.5 + 12.5 = 14\text{ clock cycles}$$
$$T_{\text{conv}} = 14 \times 83.33\text{ ns} \approx 1.17\ \mu\text{s}$$
$$F_{s,\max} = \frac{1}{1.17\ \mu\text{s}} \approx 857{,}142\text{ samples/sec (857 kSPS)}$$

With a high-impedance sensor requiring $T_{\text{sample}} = 239.5\text{ cycles}$:

$$T_{\text{conv}} = 239.5 + 12.5 = 252\text{ clock cycles} \implies 21.0\ \mu\text{s} \implies F_{s,\max} \approx 47.6\text{ kSPS}$$

---

## 5.3 Internal ADC Channels: $V_{\text{REFINT}}$, Temperature Sensor, and $V_{\text{BAT}}$

Every STM32 contains internal analog channels multiplexed into the primary ADC (typically ADC1).

### 5.3.1 Internal Voltage Reference ($V_{\text{REFINT}}$)

$V_{\text{REFINT}}$ is an internal, temperature-stable bandgap voltage reference ($V_{\text{REFINT\_TYP}} \approx 1.20\text{ V}$ on STM32F1/F4).

#### Compensating for Supply Voltage ($V_{DDA}$) Drift
When powering an STM32 from a battery or unregulated USB rail, $V_{DDA}$ may sag from $3.3\text{ V}$ down to $2.9\text{ V}$. Since the ADC uses $V_{DDA}$ as its reference, all digital conversion readings will drift upwards, reporting false physical sensor values!
To eliminate this error, sample the internal channel $V_{\text{REFINT}}$. The true operating supply voltage can be computed dynamically:

$$V_{DDA,\text{actual}} = V_{\text{REFINT\_TYP}} \times \frac{4095}{\text{Code}_{V_{\text{REFINT}}}}$$

On modern families (STM32F4/G4/L4/H7), ST factory-calibrates $V_{\text{REFINT}}$ at $3.3\text{ V}$ and stores the raw conversion code in system ROM (`VREFINT_CAL_ADDR`, typically `0x1FFF7A2A`):

$$V_{DDA,\text{actual}} = 3.3\text{ V} \times \frac{\text{*VREFINT\_CAL\_ADDR}}{\text{Code}_{V_{\text{REFINT}}}}$$

---

### 5.3.2 Internal Temperature Sensor

STM32 microcontrollers feature an internal silicon temperature sensor connected to an ADC channel (Channel 16 on STM32F1). It measures the internal junction temperature ($T_J$) of the microcontroller die.

#### Temperature Calculation Formula (STM32F103)
From the STM32F103 datasheet:
- $V_{25} = 1.43\text{ V}$ (Nominal voltage at $25^\circ\text{C}$)
- $\text{Avg\_Slope} = 4.3\text{ mV}/^\circ\text{C} = 0.0043\text{ V}/^\circ\text{C}$

$$\text{Temperature }(^\circ\text{C}) = \frac{V_{25} - V_{\text{SENSE}}}{\text{Avg\_Slope}} + 25$$

Where $V_{\text{SENSE}} = \text{Code} \times \frac{V_{DDA}}{4095}$.

---

## 5.4 ADC Acquisition Modes in STM32 HAL

**Table 5.2 — Comparison of ADC acquisition modes**

| Mode | Trigger Source | CPU Involvement | Latency & Jitter | Best Used For |
|---|---|---|---|---|
| **Polled Software** | `HAL_ADC_Start()` | 100% blocked inside `HAL_ADC_PollForConversion()` | High jitter; unpredictable | Power-on sanity checks; DC battery monitor |
| **Interrupt-Driven** | Software or Timer | Low (ISR fires on End of Conversion `EOC`) | Low jitter; high ISR overhead at $> 10\text{ kSPS}$ | Low-rate threshold detection (Analog Watchdog) |
| **Timer-Triggered DMA** | Timer TRGO Event | **0% during sampling** (Wakes only at Half/Full buffer) | **Zero software jitter** (Hardware routed) | Continuous multi-channel data acquisition, DSP, audio |

### 5.4.1 Mandatory Calibration Step
All STM32 SAR ADCs exhibit internal capacitor mismatch and voltage offsets resulting from silicon manufacturing tolerances. **Before enabling the ADC, you must run internal calibration**.

```c
/* Mandatory ADC calibration in main.c during initialization (STM32F1) */
HAL_ADCEx_Calibration_Start(&hadc1);

/* Start Timer and Circular DMA */
HAL_TIM_Base_Start(&htim3);
HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, BUFFER_SIZE);
```

---

## 5.5 Digital Sensor Interrogation: Registers, Burst Reads, and Conversions

Modern robotics systems rely heavily on digital inertial measurement units (IMUs, such as the InvenSense MPU6050 / ICM-20689), digital pressure sensors (Bosch BMP280), and magnetometers (QMC5883L).

### 5.5.1 The Sensor Initialization Lifecycle
Every robust digital sensor driver must execute a deterministic 5-step bring-up sequence:
1. **Power-On Reset Settling ($t_{\text{POR}}$)**: Wait for the sensor's internal power supply regulator and charge pump to stabilize ($30\text{ ms}$ to $100\text{ ms}$). Attempting to send I²C commands immediately upon power-up results in bus NACKs.
2. **Device Identification (`WHO_AM_I`)**: Read the read-only hardware identity register (e.g., register `0x75` on MPU6050, expected value `0x68`). If the returned byte does not match, immediately abort initialization and return `DEV_NOT_FOUND`.
3. **Power Management and Clock Source**: Take the device out of sleep mode and select a stable oscillator (e.g., PLL locked to gyro X-axis rather than the internal 8 MHz RC oscillator).
4. **Range and Filter Configuration**: Configure Full-Scale Range (e.g., Accelerometer $\pm 2g$, $\pm 4g$, $\pm 8g$, $\pm 16g$; Gyroscope $\pm 250^\circ/\text{s}$ to $\pm 2000^\circ/\text{s}$) and digital low-pass filter (DLPF) bandwidth.
5. **Interrupt Line Configuration (Optional)**: Configure the Data Ready (DRDY) pin to assert an active-high pulse whenever a new conversion completes, routing this to an STM32 EXTI interrupt pin.

---

### 5.5.2 Two's Complement Decoding and Unit Conversion

Digital sensors output physical quantities as **signed 16-bit two's complement integers** split across two 8-bit registers: `REG_HIGH` (MSB) and `REG_LOW` (LSB).

#### Bit Assembly and Sign Extension
A common bug occurs when assembling these bytes using 32-bit integers without proper casting, leading to corrupted negative values:

```c
/* CORRECT: Assemble into signed 16-bit type (handles two's complement sign extension) */
int16_t raw_accel_x = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);

/* WRONG: Buggy assembly on 32-bit platforms */
int32_t wrong_val = (buf[0] << 8) | buf[1]; /* Positive numbers only; negative numbers corrupted */
```

#### Engineering Unit Conversion (MPU6050 Example)
Once assembled, the raw integer is divided by the manufacturer's **Sensitivity Scale Factor**:
- **Accelerometer ($\pm 2g$ Range)**: Scale factor = $16{,}384\text{ LSB}/g$.
  $$a_x\ [g] = \frac{\text{raw\_accel\_x}}{16384.0f}$$
  $$a_x\ [\text{m/s}^2] = a_x\ [g] \times 9.80665\text{ m/s}^2$$
- **Gyroscope ($\pm 250^\circ/\text{s}$ Range)**: Scale factor = $131.0\text{ LSB}/(^\circ/\text{s})$.
  $$\omega_z\ [^\circ/\text{s}] = \frac{\text{raw\_gyro\_z}}{131.0f}$$
- **Temperature Sensor**:
  $$T\ [^\circ\text{C}] = \frac{\text{raw\_temp}}{340.0f} + 36.53f$$

---

## 5.6 Real-Time Acquisition Dynamics: Latency, Jitter, and Throughput

In embedded engineering, **"Real-Time" does NOT mean "real-fast"**. It means **deterministic and bounded**. A system that acquires data at $10\text{ Hz}$ with zero microsecond jitter is a true hard real-time system, whereas a system that samples at $100\text{ kHz}$ but occasionally pauses for $20\text{ ms}$ while serving an interrupt is non-real-time and will destabilize a control loop.

### 5.6.1 Throughput and Bandwidth Mathematics
When architecting high-rate acquisition systems, engineers must calculate the data generation bandwidth:

$$\text{Throughput (Bytes/sec)} = F_s \times \text{Bytes per Sample} \times \text{Number of Channels}$$

#### Example: Quad-Channel 12-bit ADC at 100 kSPS
- $F_s = 100{,}000\text{ Hz}$
- 4 channels, each storing 1 sample as a 16-bit half-word ($2\text{ bytes}$).
- $\text{Throughput} = 100{,}000 \times 2 \times 4 = 800{,}000\text{ bytes/sec} = 800\text{ kB/s}$.
- If transmitting this raw data over UART at $115{,}200\text{ baud}$ ($\approx 11.5\text{ kB/s}$), **the system will instantly overflow**. The data must either be filtered and down-sampled on-chip or streamed over high-speed USB / SPI.

---

### 5.6.2 Memory Buffering Strategies

**Table 5.3 — Embedded data buffering architectures**

| Buffer Strategy | Memory Topology | Concurrency Mechanism | Risk / Failure Mode | Best Used For |
|---|---|---|---|---|
| **Single Static Buffer** | One array | Mutex or flag | Producer overwrites while consumer reads (Data tearing) | Slow, polled DC sensors |
| **Double / Ping-Pong Buffer** | Two separate buffers ($A$ and $B$) | Producer writes $A$ while consumer reads $B$; swap upon completion | Buffer overrun if processing time exceeds buffer filling time | High-throughput block processing, DSP, FFT |
| **Circular Ring Buffer** | Fixed-size array with `head` and `tail` pointers | Atomic pointer arithmetic (`modulo`) | Queue overflow if consumer stalls | Byte streams, UART RX/TX, event logs |

---

## 5.7 Digital Signal Processing: Anti-Aliasing and Filtering

### 5.7.1 The Nyquist-Shannon Sampling Theorem and Anti-Aliasing

The Nyquist-Shannon theorem states that to reconstruct a continuous signal of maximum bandwidth $f_{\max}$ without distortion, the sampling rate $F_s$ must satisfy:

$$F_s \ge 2 \times f_{\max}$$

If the analog signal contains high-frequency noise components above the **Nyquist frequency** ($f_{\text{Nyquist}} = \frac{F_s}{2}$)—such as $20\text{ kHz}$ motor PWM switching noise—those frequencies will **fold back (alias)** into the low-frequency spectrum, appearing as fake low-frequency oscillations that cannot be filtered in software!
- **Engineering Rule**: Always place a physical **analog RC low-pass filter** on the ADC pin before sampling. Select the cutoff frequency $f_c = \frac{1}{2\pi R C}$ to attenuate noise above $\frac{F_s}{2}$.

---

### 5.7.2 Moving Average vs. Exponential Moving Average (EMA)

Once samples are safely digitized, software filters eliminate residual high-frequency sensor noise.

#### 1. Moving Average Filter (Window Size $M$)
Averages the last $M$ samples. Acts as a linear finite impulse response (FIR) filter with a sinc-like frequency response:

$$y[n] = \frac{1}{M} \sum_{k=0}^{M-1} x[n-k]$$

*Drawback*: Requires maintaining a history ring-buffer of $M$ elements and adds significant phase lag ($T_{\text{lag}} \approx \frac{M-1}{2 F_s}$).

#### 2. Exponential Moving Average (EMA / IIR Filter)
An infinite impulse response (IIR) single-pole filter that requires **zero array storage**—only one state variable:

$$y[n] = \alpha \cdot x[n] + (1 - \alpha) \cdot y[n-1]$$

Where $\alpha \in (0, 1]$ is the smoothing factor. Smaller $\alpha$ provides heavier filtering at the expense of step response latency.

**Listing 5.1 — High-efficiency fixed-point integer EMA filter (Zero floating-point overhead)**

```c
/* Fixed-point EMA implementation using Q15 arithmetic (Scale = 32768) */
#define EMA_ALPHA_Q15   3277  /* alpha = 3277 / 32768 ≈ 0.10 */

int32_t EMA_Filter_Q15(int32_t current_sample, int32_t *state_q15)
{
    /* y[n] = y[n-1] + alpha * (x[n] - y[n-1]) */
    int32_t diff = current_sample - (*state_q15 >> 15);
    *state_q15 += (int32_t)(((int64_t)diff * EMA_ALPHA_Q15));
    return *state_q15 >> 15;
}
```

---

## 5.8 Cortex-M7 and STM32H7 Data-Cache (D-Cache) Coherency

On high-performance STM32 microcontrollers featuring ARM Cortex-M7 cores (such as the STM32F7 and STM32H7 families running at up to 480–550 MHz), the core includes high-speed **L1 Data Cache (D-Cache)**. The D-Cache transparently accelerates memory reads and writes by keeping local copies of SRAM in 32-byte cache lines.

However, the **DMA controller operates independently of the CPU core** and accesses physical SRAM directly over the internal AXI/AHB bus matrix without passing through the L1 cache. This creates the classic **DMA Cache Incoherency Hazard**.

```text
       ┌───────────────┐
       │ ARM Cortex-M7 │
       │     Core      │
       └───────┬───────┘
               │ Reads/Writes
       ┌───────▼───────┐
       │   L1 D-Cache  │  ◄── CPU sees cached data!
       └───────┬───────┘
               │ Eviction / Fetch
═══ AXI / AHB Bus Matrix ══════════════════════════════════════════
       ▲               ▲
       │ DMA Writes    │ Reads/Writes
┌──────┴──────┐ ┌──────┴──────┐
│DMA Controller│ │ Physical RAM│  ◄── Physical SRAM updated by DMA!
└─────────────┘ └─────────────┘
```

### The Two Incoherency Hazards
1. **DMA Reception Hazard (DMA to Memory)**:
   The DMA writes fresh sensor samples into physical SRAM. However, the CPU's D-Cache still holds the old data for that address range. When application code reads the buffer, it reads the stale cache line, completely missing the new sensor data!
2. **DMA Transmission Hazard (Memory to Peripheral)**:
   The CPU writes telemetry data into an SRAM array. Because D-Cache uses a **write-back policy**, the data remains sitting in the CPU's internal cache and has not yet been flushed to physical SRAM. When the DMA starts transmitting, it reads stale garbage from physical RAM.

---

### The Three Architectural Solutions

#### Solution 1: Cache Maintenance Operations (Clean and Invalidate)
Use CMSIS cache control intrinsics before and after every DMA operation:
- **Before DMA Transmit**: Clean the cache (flush dirty cache lines out to physical RAM):
  ```c
  /* Ensure physical RAM holds the latest data written by the CPU */
  SCB_CleanDCache_by_Addr((uint32_t *)tx_buf, TX_BUF_SIZE);
  HAL_UART_Transmit_DMA(&huart1, tx_buf, TX_BUF_SIZE);
  ```
- **After DMA Receive**: Invalidate the cache (force CPU to discard cache lines and reload from physical RAM):
  ```c
  void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
  {
      /* Invalidate CPU cache so core reads fresh data placed by DMA */
      SCB_InvalidateDCache_by_Addr((uint32_t *)rx_buf, RX_BUF_SIZE);
      App_ProcessPacket(rx_buf, RX_BUF_SIZE);
  }
  ```

> [!CAUTION]
> **The 32-Byte Cache Line Alignment Rule**: Cache operations operate strictly on **32-byte cache lines**. If a DMA buffer is not aligned to a 32-byte boundary, or if its length is not a multiple of 32 bytes, invalidating the buffer will unintentionally destroy adjacent CPU variables allocated in the same cache line!
> Always declare DMA buffers on Cortex-M7 with 32-byte alignment:
> ```c
> uint8_t rx_buf[256] __attribute__((aligned(32)));
> ```

#### Solution 2: Non-Cacheable Memory Regions via MPU (Memory Protection Unit)
The most robust solution is to configure the Cortex-M7 MPU to designate a dedicated SRAM section (such as SRAM4 or D2 domain RAM) as **Non-Cacheable (Shareable Device Memory)**. All DMA buffers are placed inside this region using linker attributes:
```c
uint8_t dma_buffer[1024] __attribute__((section(".dma_buffer_section")));
```
Because the CPU never caches this memory region, cache coherency hazards are completely eliminated by hardware design.
