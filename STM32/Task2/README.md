# Assiut Robotics Team - Embedded Systems Batch

## Task 2: Multi-Channel ADC with DMA & LCD Display

---

### 1. Theoretical Questions & Answers

#### Q1: What is ADC and DMA?

- **ADC (Analog-to-Digital Converter):**
  An on-chip peripheral that samples continuous real-world analog physical quantities (e.g., voltage from a sensor or potentiometer between $0\text{ V}$ and $V_{REF+} = 3.3\text{ V}$) and quantizes them into a discrete digital integer value.
  On the STM32F103C8T6, the ADC is a **12-bit Successive Approximation Register (SAR)** converter with conversion results ranging from $0$ to $4095$ ($2^{12} - 1$). The analog voltage is calculated as:
  $$V_{IN} = \frac{\text{ADC\_Value}}{4095} \times V_{REF}$$
- **DMA (Direct Memory Access):**
  A high-speed hardware bus master coprocessor that manages data transfers between peripherals (ADC, SPI, UART, I2C, Timers) and memory (SRAM), or memory-to-memory, **without any CPU core intervention**. The DMA controller takes over the AHB system bus during transfers, allowing the CPU to execute instructions in parallel or enter sleep mode.

---

#### Q2: Why do we use DMA (especially with ADC)?

1. **Single Regular Data Register Limitation (`ADC_DR`):**
   In the STM32F103, there is only **one single 16-bit Regular Data Register** (`ADC_DR`) shared across all 16 regular channels. When performing multi-channel scanning (e.g., reading 4 potentiometers sequentially), each converted channel immediately overwrites `ADC_DR`. Without DMA, software polling or interrupt routines cannot read `ADC_DR` fast enough between channels, leading to overrun and corrupted data.
2. **Zero CPU Overhead:**
   Moving data from `ADC_DR` to memory is handled entirely in hardware at AHB bus clock speeds. The CPU is completely freed from servicing polling loops or interrupt context switches.
3. **Circular Buffering (`DMA_CIRCULAR`):**
   The DMA controller continuously and autonomously refreshes the destination array in memory (`adc_dma_buffer[0..3]`). The application always has instant, non-blocking access to the latest real-time sensor measurements.
4. **Power Savings:**
   The MCU core can remain in low-power modes (e.g., Sleep mode) while data transfers proceed uninterrupted in the background.

---

#### Q3: How can I compare two ADCs and decide which is better?

To evaluate and select an ADC for an engineering application, compare the following parameters:

| Parameter | What It Measures | Why It Matters |
| :--- | :--- | :--- |
| **Resolution** | Number of output bits ($N$). | Determines the voltage resolution ($\text{LSB} = \frac{V_{REF}}{2^N}$). 12-bit gives 4096 levels ($0.8\text{ mV}$ step at $3.3\text{ V}$); 16-bit gives 65536 levels. |
| **Sampling Rate (Throughput)** | Conversions per second (SPS, kSPS, MSPS). | Determines the highest signal frequency that can be accurately captured without aliasing ($f_{signal} < \frac{f_{sampling}}{2}$, Nyquist Theorem). STM32F103 achieves up to 1 MSPS. |
| **Architecture** | SAR, Sigma-Delta ($\Delta\Sigma$), Flash, Pipelined. | SAR offers balanced speed and resolution (general MCU sensors); Sigma-Delta gives ultra-high precision (24-bit, load cells/temperature); Flash gives extreme speed (GSPS, RF/oscilloscopes). |
| **ENOB (Effective Number of Bits)** | Real-world resolution considering noise and distortion. | $\text{ENOB} = \frac{\text{SINAD} - 1.76}{6.02}$. A nominal 12-bit ADC often has an ENOB of $\approx 10.8\text{ bits}$. |
| **Static Accuracy (DNL & INL)** | Differential & Integral Non-Linearity. | DNL measures step consistency (prevents missing codes); INL measures total curvature/deviation from an ideal straight line. |
| **Dynamic Performance (SNR, THD, SINAD)** | Noise floor, harmonic distortion, and spurious signals. | Essential for AC signals, audio processing, vibration analysis, and communications. |
| **Input Channels & Modes** | Single-ended vs. Differential, PGA presence. | Differential inputs cancel common-mode noise; built-in Programmable Gain Amplifiers (PGA) allow reading millivolt signals without external op-amps. |
| **Power Consumption** | Power consumed during conversion and standby ($\mu\text{W}$). | Critical for battery-powered and energy-harvesting IoT nodes. |

---

### 2. Implementation Details

#### Pin Mapping

| Peripheral | Pin | Function | Description |
| :--- | :--- | :--- | :--- |
| **Potentiometer 1** | `PA3` | `ADC1_IN3` | Channel 3, Rank 1 in Scan Sequence |
| **Potentiometer 2** | `PA4` | `ADC1_IN4` | Channel 4, Rank 2 in Scan Sequence |
| **Potentiometer 3** | `PA5` | `ADC1_IN5` | Channel 5, Rank 3 in Scan Sequence |
| **Potentiometer 4** | `PA6` | `ADC1_IN6` | Channel 6, Rank 4 in Scan Sequence |
| **LCD Enable** | `PA0` | GPIO Output | LCD `EN` strobe pulse |
| **LCD Reg Select** | `PA1` | GPIO Output | LCD `RS` (0 = Command, 1 = Data) |
| **LCD Read/Write** | `PA2` | GPIO Output | LCD `RW` (0 = Write) |
| **LCD Data Bus** | `PB0`..`PB7` | GPIO Output | LCD 8-bit Data Bus `D0`..`D7` |

#### LCD Format & Resistance Display

- **Row 1:** `P1:xxxx  P2:xxxx`
- **Row 2:** `P3:xxxx  P4:xxxx`
- Each potentiometer value `xxxx` displays the **actual resistance** computed from the 12-bit ADC reading and nominal pot value:
  $$R = \frac{\text{ADC\_Value}}{4095} \times R_{\text{nominal}}$$
- **Configurable Potentiometers:**
  - `P1`: $1\text{ k}\Omega$ ($1,000\,\Omega$)
  - `P2`: $10\text{ k}\Omega$ ($10,000\,\Omega$)
  - `P3`: $100\text{ k}\Omega$ ($100,000\,\Omega$)
  - `P4`: $1\text{ M}\Omega$ ($1,000,000\,\Omega$)
- **4-Character Truncation & Formatting Rules:**
  - Strictly limited to $\le 4$ characters per value (truncated at index 4 if longer) to preserve the exact 16-character LCD line.
  - Left-aligned with space padding (`%-4s`) so LCD column coordinates never shift or jitter.
  - Examples:
    - $52\%$ on $1\text{ k}\Omega$ pot: displays `520` (actual resistance $520\,\Omega$)
    - $52\%$ on $10\text{ k}\Omega$ pot: displays `5.2K` (actual resistance $5.2\text{ k}\Omega$)
    - $52\%$ on $100\text{ k}\Omega$ pot: displays `52K` (actual resistance $52\text{ k}\Omega$)
    - $52\%$ on $1\text{ M}\Omega$ pot: displays `520K` (actual resistance $520\text{ k}\Omega$)
    - $100\%$ on $1\text{ k}\Omega$ pot: displays `1000`
    - $100\%$ on $10\text{ k}\Omega$ pot: displays `10K`
    - $100\%$ on $100\text{ k}\Omega$ pot: displays `100K`
    - $100\%$ on $1\text{ M}\Omega$ pot: displays `1.0M`
    - $0\%$ on any pot: displays `0`

#### Proteus CPU Load Optimization

To prevent "Simulation is not running in real time due to CPU load" warnings in Proteus 8:

1. **DMA IRQ Disabled (`HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn)`):**
   In hardware Circular DMA mode, DMA autonomously transfers conversion results into SRAM (`adc_dma_buffer`) without needing CPU interrupt servicing. Disabling the interrupt eliminates thousands of unnecessary ISR context switches per second.
2. **Microsecond Delays in LCD Driver:**
   Replaced blocking millisecond delays (`HAL_Delay(1)`) in `LCD_PulseEnable` and `LCD_SendData` with calibrated microsecond delays (`LCD_DelayUs(5)` and `LCD_DelayUs(40)`). This slashed LCD refresh CPU time from $\approx 100\text{ ms}$ down to $< 2\text{ ms}$.
3. **ADC Clock Prescaler:**
   Configured to `RCC_ADCPCLK2_DIV6` ($1.33\text{ MHz}$), keeping the Proteus analog SPICE solver light and efficient.

---

### 3. Build & Simulation Instructions

#### Building the Firmware

Run the provided build script:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Output files:

- `STM32F103C8TX.elf`
- `STM32F103C8TX.hex`

#### Simulating in Proteus

1. Open [`Task2-Proteus.pdsprj`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task2/STM32F103C8TX/STM32F103C8TX/Task2-Proteus.pdsprj) in Proteus 8.
2. The STM32 microcontroller is already configured to load `Debug\STM32F103C8TX.hex`.
3. Press **Run Simulation**:
   - The LCD initializes and displays the welcome message:

     ```
     Assiut Robotics 
     Task 2: ADC+DMA 
     ```

   - After 1.2 seconds, the display shows the live potentiometer resistances:

     ```
     P1:520   P2:5.2K 
     P3:52K   P4:520K 
     ```

   - Adjusting any of the 4 potentiometers in Proteus updates the displayed values in real time with zero flicker and low CPU load.
