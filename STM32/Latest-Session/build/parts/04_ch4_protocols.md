# 4. Part 3 — Serial Communication Protocols: UART, SPI, and I²C

Embedded systems rarely operate in isolation. Microcontrollers must exchange telemetry, sensor data, actuator setpoints, and diagnostic messages with host computers, companion processors, displays, and peripheral ICs. Rather than running parallel 8-bit or 16-bit buses across circuit boards—which consume excessive PCB area, cause electromagnetic interference, and waste GPIO pins—modern microcontrollers rely on **serial communication protocols**.

This chapter provides a rigorous, hardware-level and HAL-level analysis of the three foundational serial protocols used in STM32 microcontrollers: **UART/USART**, **SPI**, and **I²C**. We analyze their physical layer physics, frame architectures, clock synchronization, driver implementations (Polling, Interrupt, and DMA), error management, and hardware recovery procedures, culminating in an objective protocol selection matrix.

![Figure 4.1 — Serial communication protocol physical layers and frame structures](diagrams/fig5_serial_protocols.png)
*Figure 4.1 — Serial communication architectures: (A) Asynchronous UART with framing and start/stop bits; (B) Synchronous SPI four-wire full-duplex bus with CS gating; (C) Synchronous I²C open-drain two-wire bus with pull-up resistors, START/STOP conditions, and 9-bit clock packets.*

---

## 4.1 UART / USART (Universal Asynchronous / Synchronous Receiver Transmitter)

### 4.1.1 Operating Principle and Physics

UART (Universal Asynchronous Receiver-Transmitter) is a point-to-point, full-duplex communication interface that transmits data sequentially bit-by-bit without sharing a dedicated clock signal. Because there is no common clock line between the transmitter (TX) and receiver (RX), both devices must agree beforehand on four core parameters:
1. **Baud Rate**: The transmission speed in symbols (bits) per second (e.g., 9600, 115200, 921600 bps).
2. **Word Length**: The number of payload data bits (typically 8 bits, occasionally 7 or 9 bits).
3. **Parity**: Error-detection bit appended to the data (None, Even, or Odd).
4. **Stop Bits**: The duration of the line idle state concluding the frame (typically 1 or 2 bits).

The standard idle state of a UART transmission line is logic **HIGH** ($V_{DD}$, typically 3.3 V on STM32).

#### The UART Frame Structure
Every transmitted byte is wrapped inside a deterministic packet envelope:
1. **Start Bit (Logic LOW)**: The transmitter drives the line from HIGH to LOW for exactly 1 bit period. This high-to-low transition alerts the receiver's internal shift register that a byte is incoming and triggers its baud rate synchronization timer.
2. **Data Bits (LSB First)**: The payload bits (usually 8 bits, $D_0$ through $D_7$) are clocked out sequentially at the nominal bit duration $T_{\text{bit}} = \frac{1}{\text{Baud}}$.
3. **Parity Bit (Optional)**: If enabled, an extra bit is inserted to make the total count of '1's even (Even Parity) or odd (Odd Parity).
4. **Stop Bit (Logic HIGH)**: The transmitter drives the line back to logic HIGH for 1, 1.5, or 2 bit periods. This guarantees a high state before the next frame's start bit can generate a falling edge.

```text
Idle (HIGH) ──┐      ┌───┬───┬───┬───┬───┬───┬───┬───┐      ┌─── Idle (HIGH)
              │START │D0 │D1 │D2 │D3 │D4 │D5 │D6 │D7 │[PAR] │STOP
              └──────┴───┴───┴───┴───┴───┴───┴───┴───┴──────┘
Bit Time:      ◄─Tb─►
```

#### USART vs. UART
A standard STM32 **UART** supports asynchronous serial communication. A **USART** (Universal Synchronous/Asynchronous Receiver Transmitter) includes additional hardware logic to generate and accept a synchronous master/slave clock signal (CK pin), support Smartcard interfaces (ISO 7816), IrDA SIR ENDEC, LIN (Local Interconnect Network), and Modbus protocols. When synchronous mode is not enabled in CubeMX, USART peripherals operate identically to UARTs.

---

### 4.1.2 Baud Rate Generation and Clock Tolerances

Inside the STM32 UART peripheral, incoming and outgoing bits are sampled using an oversampling clock generated from the peripheral bus clock ($f_{\text{PCLK1}}$ or $f_{\text{PCLK2}}$) divided by the **Baud Rate Register (USART_BRR)**.

#### Oversampling Architecture (16× vs. 8×)
Most STM32 UART peripherals default to **16× oversampling** (`USART_CR1_OVER8 = 0`). In this mode:
- The receiver divides each bit period into 16 discrete time slices.
- When a falling edge (Start bit) is detected, the receiver verifies the line state at slices 3, 5, and 7 to reject high-frequency noise spikes.
- Subsequent data bits are sampled at slices **8, 9, and 10**. A majority vote (at least 2 out of 3 samples identical) determines the registered bit value.
- On higher-performance families (STM32F4/F7/G4/H7), **8× oversampling** can be selected to achieve double the maximum baud rate, at the expense of reduced tolerance to clock jitter.

#### Baud Rate Calculation (STM32F1 / Classic USART)
The baud rate formula in 16× oversampling mode is:

$$\text{Baud} = \frac{f_{\text{PCLK}}}{16 \times \text{USART\_BRR}}$$

Where $\text{USART\_BRR}$ is an unsigned fixed-point number:
- Bits `[15:4]` contain the 12-bit integer **Mantissa**.
- Bits `[3:0]` contain the 4-bit **Fraction** (scaled by $\frac{1}{16}$).

$$\text{USART\_BRR} = \text{Mantissa} + \frac{\text{Fraction}}{16}$$

#### Worked Calculation: 115,200 Baud on STM32F103 (USART2 on APB1)
Assume an STM32F103 running at 72 MHz SYSCLK with APB1 prescaler = 2 ($f_{\text{PCLK1}} = 36\text{ MHz}$).
1. Calculate theoretical divisor:
   $$\text{Divisor} = \frac{36{,}000{,}000}{16 \times 115{,}200} = \frac{36{,}000{,}000}{1{,}843{,}200} \approx 19.53125$$
2. Extract integer mantissa:
   $$\text{Mantissa} = \lfloor 19.53125 \rfloor = 19 = \text{0x13}$$
3. Extract fractional part:
   $$\text{Fraction} = \text{round}(0.53125 \times 16) = \text{round}(8.5) = 9 = \text{0x9}$$
4. Register value:
   $$\text{USART\_BRR} = (19 \ll 4) | 9 = \text{0x0139}$$
5. Calculate actual baud rate and error:
   $$\text{Actual Baud} = \frac{36{,}000{,}000}{16 \times \left(19 + \frac{9}{16}\right)} = \frac{36{,}000{,}000}{16 \times 19.5625} = \frac{36{,}000{,}000}{313} \approx 115{,}015.97\text{ bps}$$
   $$\text{Error} = \frac{115{,}015.97 - 115{,}200}{115{,}200} \times 100\% \approx -0.16\%$$

> [!NOTE]
> **Clock Tolerance Limit**: Asynchronous serial communication requires cumulative transmitter and receiver clock frequency error to remain strictly below **$\pm 2.5\%$** (and ideally below $\pm 1.5\%$) across the entire 10-bit frame. If error exceeds $2.5\%$, the 10th bit (Stop bit) will be sampled during the adjacent bit transition, causing catastrophic framing errors (`FE`). Because internal RC oscillators (HSI) drift by $\pm 1\%$ to $\pm 3\%$ over temperature, **always use an external crystal oscillator (HSE) for reliable UART communication above 115,200 baud**.

---

### 4.1.3 HAL Implementation: Polling, Interrupt, and DMA Modes

STM32 HAL provides three distinct execution paradigms for UART transmission and reception.

#### Mode 1: Polling (Blocking)
The CPU loops continuously polling the Transmit Data Register Empty (`TXE`) or Read Data Register Not Empty (`RXNE`) status flags until transfer completes or a timeout expires.

```c
/* Blocking transmit: CPU halted for ~87 µs per character at 115200 baud */
uint8_t msg[] = "SENSOR_OK\r\n";
HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, msg, sizeof(msg) - 1, 100);
if (status != HAL_OK) {
    /* Handle timeout or hardware bus error */
}
```
*Application*: Power-on self-test (POST), panic logging inside `HardFault_Handler`, single-byte command consoles during initialization.

#### Mode 2: Interrupt-Driven (Non-Blocking Byte-by-Byte)
The CPU initiates the transfer and immediately returns to application tasks. The UART hardware triggers an interrupt upon every received byte (`RXNE`) or transmitted byte (`TXE`/`TC`). The HAL driver manages internal buffer pointers in the ISR and invokes a high-level callback upon completion.

```c
/* Start non-blocking reception of a 1-byte command */
uint8_t rx_char;
HAL_UART_Receive_IT(&huart2, &rx_char, 1);

/* HAL weak callback override */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        ProcessCommandByte(rx_char);
        /* Re-arm receiver for the next byte */
        HAL_UART_Receive_IT(&huart2, &rx_char, 1);
    }
}
```
*Application*: Interactive human command shells, low-bandwidth ASCII streams (< 1000 bytes/sec).
*Drawback*: At 921,600 baud, an interrupt fires every $10.85\ \mu\text{s}$. If context switching and HAL ISR overhead consume $3\ \mu\text{s}$, the CPU suffers **28% utilization** purely serving UART interrupts.

#### Mode 3: DMA Mode (Zero-CPU High-Throughput Streaming)
The DMA controller routes bytes directly between the peripheral data register (`USART_DR` or `USART_RDR`) and an SRAM buffer without CPU involvement.

```c
/* Transmit a 512-byte telemetry packet via DMA */
uint8_t telemetry_buf[512];
FillTelemetry(telemetry_buf, sizeof(telemetry_buf));

HAL_UART_Transmit_DMA(&huart2, telemetry_buf, sizeof(telemetry_buf));
/* CPU immediately continues executing sensor control algorithms */
```

---

### 4.1.4 The UART Stream Problem: Framing and Packet Delineation

UART transmits an unformatted stream of bytes. Unlike SPI or I²C, there is no hardware packet boundary or Slave Select edge. The application receiver must reconstruct discrete messages from continuous bytes.

**Table 4.1 — UART message framing strategies**

| Framing Technique | Operating Principle | Advantages | Disadvantages | Best Used For |
|---|---|---|---|---|
| **Delimiter / Newline** | Stream ends with `\r\n` or `\0` | Human readable, simple string parsing | Cannot transmit raw binary (data may contain delimiter) | ASCII CLI, AT commands, NMEA GPS |
| **Fixed-Length Packet** | Every message is exactly $N$ bytes | Trivial buffer management, zero parser state | Inflexible; padding wastes bandwidth | Periodic sensor packets (e.g., 14-byte IMU) |
| **Length-Prefix Header** | Header contains `[SYNC][LEN][PAYLOAD][CRC]` | Flexible binary payloads; high efficiency | Requires state-machine parser; sync loss recovery needed | Binary telemetry, inter-MCU links |
| **COBS / SLIP Encoding** | Byte stuffing replaces zero bytes; `0x00` marks boundary | Arbitrary binary packets; unambiguous packet delimitation | Encoding/decoding CPU overhead (~1-2%) | Robust robotic communication, ROSserial |
| **UART IDLE Line Detection** | Hardware triggers interrupt when RX line idles for 1 frame | Automatic variable-length reception with DMA | Sensitive to inter-byte transmission pauses | High-speed telemetry, unknown length packets |

#### The Modern Solution: Circular DMA with IDLE Line Detection (`ReceiveToIdle`)
Starting with STM32Cube HAL v1.8+, ST introduced the `HAL_UARTEx_ReceiveToIdle_DMA()` API. It utilizes the hardware **IDLE flag** (`USART_SR_IDLE` or `USART_ISR_IDLE`), which asserts when the RX pin remains HIGH for at least one full character frame duration after receiving data.

**Listing 4.1 — Robust variable-length packet reception via DMA and IDLE detection**

```c
#define RX_BUFFER_SIZE  256
uint8_t dma_rx_buf[RX_BUFFER_SIZE];

void App_UART_Init(void)
{
    /* Start DMA listening in normal or circular mode */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, dma_rx_buf, RX_BUFFER_SIZE);
    /* Disable half-transfer interrupt if only complete packets are needed */
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
}

/* Invoked when either:
   1. The DMA buffer fills completely (Size == RX_BUFFER_SIZE)
   2. The transmitter stops sending bytes (IDLE line event; Size < RX_BUFFER_SIZE) */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2) {
        /* Process received packet of exact length 'Size' */
        App_ProcessPacket(dma_rx_buf, Size);

        /* Re-arm the DMA reception */
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, dma_rx_buf, RX_BUFFER_SIZE);
        __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
    }
}
```

---

### 4.1.5 UART Hardware Errors and Failure Modes

The UART peripheral status register monitors transmission integrity through four dedicated error flags:
1. **Overrun Error (`ORE`)**: A new byte arrived in the shift register while the previous byte was still unread in the Data Register (`RDR`/`DR`). 
   - *Cause*: Interrupts disabled too long, or DMA stream stopped while data was flowing.
   - *Impact*: In many STM32 families (such as STM32F1), an unhandled `ORE` flag **freezes the receiver** and completely halts all subsequent reception!
2. **Framing Error (`FE`)**: The expected Stop bit was sampled as logic LOW instead of HIGH.
   - *Cause*: Baud rate mismatch (> 2.5%), noisy ground line, or mismatched word length/parity settings.
3. **Noise Error (`NE`)**: Slices 8, 9, and 10 of a bit period produced conflicting logic levels.
   - *Cause*: Electrical coupling from motor PWM lines or unshielded cables.
4. **Parity Error (`PE`)**: The calculated parity of data bits failed to match the received parity bit.

**Listing 4.2 — Comprehensive UART error handler callback**

```c
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uint32_t err = HAL_UART_GetError(huart);

    if (err & HAL_UART_ERROR_ORE) {
        /* Clear Overrun flag by reading SR followed by DR (F1/F4) or ICR (G0/G4/H7) */
        __HAL_UART_CLEAR_OREFLAG(huart);
    }
    if (err & HAL_UART_ERROR_FE) {
        __HAL_UART_CLEAR_FEFLAG(huart);
    }
    if (err & HAL_UART_ERROR_NE) {
        __HAL_UART_CLEAR_NEFLAG(huart);
    }
    if (err & HAL_UART_ERROR_PE) {
        __HAL_UART_CLEAR_PEFLAG(huart);
    }

    /* Abort jammed DMA transfer and re-arm reception */
    HAL_UART_AbortReceive(huart);
    HAL_UARTEx_ReceiveToIdle_DMA(huart, dma_rx_buf, RX_BUFFER_SIZE);
}
```

---

### 4.1.6 Retargeting `printf` via UART

Standard C library `printf` output can be routed directly to an STM32 UART console by implementing the low-level character output hook.

**Listing 4.3 — Retargeting standard I/O to USART2 with GCC / Newlib-nano**

```c
#include <stdio.h>

/* GCC / STM32CubeIDE newlib-nano retarget prototype */
int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    /* Transmit single character with 10 ms timeout */
    HAL_UART_Transmit(&huart2, &c, 1, 10);
    return ch;
}

/* Optional: Support _write for POSIX compliance */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, 100);
    return len;
}
```

> [!TIP]
> **Float Formatting Support**: In STM32CubeIDE, `printf("%f")` is disabled by default to save flash memory. To enable floating-point printing, navigate to:  
> **Project Properties → C/C++ Build → Settings → Tool Settings → MCU Settings → Check "Use float with printf from newlib-nano (-u _printf_float)"**.

---

## 4.2 SPI (Serial Peripheral Interface)

### 4.2.1 Operating Principle and Architecture

SPI is a synchronous, four-wire, master-slave, full-duplex serial interface engineered for high-bandwidth data transfer (up to 50+ MHz on advanced STM32 families). Communication is orchestrated entirely by a single **Master** device, which generates the serial clock ($SCLK$) and selects peripheral **Slave** devices using individual Chip Select ($CS$ or $NSS$) lines.

#### Bus Lines
1. **SCLK (Serial Clock)**: Driven exclusively by the Master. Synchronizes bit shifting.
2. **MOSI (Master Out Slave In)**: Data line driven by Master; carries commands and write data to Slaves.
3. **MISO (Master In Slave Out)**: Data line driven by the selected Slave; carries response and read data to Master.
4. **NSS / CS (Slave Select / Chip Select)**: Active-low line driven by the Master to enable a specific slave device. Unselected slaves place their MISO pin in high-impedance (tri-state) mode.

```text
 Master (STM32)                       Slave Device (e.g. Flash / IMU)
┌──────────────┐                     ┌──────────────┐
│         SCLK ├────────────────────►│ SCLK         │
│         MOSI ├────────────────────►│ MOSI (SDI)   │
│         MISO │◄────────────────────┤ MISO (SDO)   │
│   GPIO (NSS) ├────────────────────►│ CS           │
└──────────────┘                     └──────────────┘
```

---

### 4.2.2 The Shift Register Circular Ring

At the silicon level, SPI communication operates as a **circular dual shift-register ring**. During every clock pulse on SCLK:
1. The Master shifts one bit out of its internal shift register onto **MOSI**, while simultaneously shifting one bit in from **MISO**.
2. The Slave shifts one bit out of its internal shift register onto **MISO**, while simultaneously shifting one bit in from **MOSI**.

```text
       Master Shift Register                 Slave Shift Register
     ┌───┬───┬───┬───┬───┬───┐             ┌───┬───┬───┬───┬───┬───┐
 ┌───┤ 7 │ 6 │ 5 │ 4 │ 3 │ 0 │◄─── MISO ───┤ 7 │ 6 │ 5 │ 4 │ 3 │ 0 │◄───┐
 │   └───┴───┴───┴───┴───┴───┘             └───┴───┴───┴───┴───┴───┘    │
 └──────────────── MOSI ────────────────────────────────────────────────┘
```

> [!IMPORTANT]
> **Full-Duplex Implication and Dummy Clocks**: In SPI, **transmission and reception occur simultaneously**. You cannot read a byte from a slave without transmitting a byte, and you cannot transmit a byte without receiving a byte. When an STM32 Master desires to read sensor data, it must actively clock the bus by transmitting **dummy bytes** (typically `0x00` or `0xFF`).

---

### 4.2.3 Clock Polarity (CPOL) and Clock Phase (CPHA): The Four SPI Modes

SPI devices specify their timing requirements using two configuration bits: **CPOL** and **CPHA**.

- **CPOL (Clock Polarity)**: Defines the idle logic level of SCLK when no transfer is active.
  - `CPOL = 0`: Clock line idles LOW.
  - `CPOL = 1`: Clock line idles HIGH.
- **CPHA (Clock Phase)**: Defines which clock edge captures (samples) data and which edge shifts (toggles) data.
  - `CPHA = 0`: Data is captured on the **first** clock transition and toggled on the second.
  - `CPHA = 1`: Data is toggled on the first clock transition and captured on the **second**.

**Table 4.2 — The four standard SPI modes**

| SPI Mode | CPOL (Idle Level) | CPHA (Capture Edge) | Clock Transition for Capture | Waveform Character |
|---|---|---|---|---|
| **Mode 0 (0,0)** | LOW (0) | 1st Edge (0) | Rising Edge | Most common for microcontrollers, sensors, and flash |
| **Mode 1 (0,1)** | LOW (0) | 2nd Edge (1) | Falling Edge | Common in industrial ADCs and motor drivers |
| **Mode 2 (1,0)** | HIGH (1) | 1st Edge (0) | Falling Edge | Infrequently used |
| **Mode 3 (1,1)** | HIGH (1) | 2nd Edge (1) | Rising Edge | Common alternative for high-speed Flash/FRAM |

```text
Mode 0 (CPOL=0, CPHA=0):
SCLK: Idle LOW ───┐     ┌─────┐     ┌─────┐     └─── Idle LOW
                  │  1  │  2  │  3  │  4  │  5
Data Sampled:     ▲     ▲     ▲     ▲     ▲ (First / Rising edge)

Mode 3 (CPOL=1, CPHA=1):
SCLK: Idle HIGH ──┐     ┌─────┐     ┌─────┐     ┌─── Idle HIGH
                  │  1  │  2  │  3  │  4  │  5  │
Data Sampled:           ▲           ▲           ▲ (Second / Rising edge)
```

---

### 4.2.4 Hardware NSS vs. Software Chip Select (CS)

CubeMX allows configuring the SPI slave select pin as **Hardware NSS** or **Software CS**.

#### Why Software CS is the Universal Engineering Choice
- **Hardware NSS Pitfall**: In STM32 SPI master hardware mode, the Hardware NSS pin automatically drives LOW at peripheral enable, but **pulses high or stays low unpredictably between bytes** depending on family errata and FIFO configurations. Most external SPI sensors require CS to remain continuously LOW across a multi-byte command/data burst, and toggle HIGH only when the entire transaction completes.
- **Multi-Slave Limitation**: Hardware NSS supports exactly **one** slave per SPI peripheral. A system with an IMU, an SD card, and an external flash would require three separate SPI peripherals.
- **The Solution**: In CubeMX, set **NSS = Disable (Software)**. Configure standard GPIO output pins (e.g., `PB12`, `PA4`, `PC4`) as Chip Select lines, pulling them LOW before a transmission and returning them HIGH after completion.

**Listing 4.4 — Reusable SPI register read/write transaction with Software CS**

```c
#define CS_GPIO_PORT    GPIOB
#define CS_PIN          GPIO_PIN_12

#define CS_SELECT()     HAL_GPIO_WritePin(CS_GPIO_PORT, CS_PIN, GPIO_PIN_RESET)
#define CS_DESELECT()   HAL_GPIO_WritePin(CS_GPIO_PORT, CS_PIN, GPIO_PIN_SET)

/* Read an 8-bit register from an SPI sensor */
uint8_t SPI_ReadRegister(SPI_HandleTypeDef *hspi, uint8_t reg_addr)
{
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    /* Most SPI sensors specify MSB=1 in the register address byte for READ */
    tx_buf[0] = reg_addr | 0x80; 
    tx_buf[1] = 0xFF; /* Dummy byte to clock in the response */

    CS_SELECT();
    HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 2, 50);
    CS_DESELECT();

    return rx_buf[1]; /* Received sensor payload */
}

/* Burst read multiple bytes (e.g., 6-axis IMU raw data) */
HAL_StatusTypeDef SPI_ReadBurst(SPI_HandleTypeDef *hspi, uint8_t start_reg, 
                               uint8_t *dest, uint16_t length)
{
    uint8_t cmd = start_reg | 0x80;
    HAL_StatusTypeDef status;

    CS_SELECT();
    /* Send register command */
    status = HAL_SPI_Transmit(hspi, &cmd, 1, 10);
    if (status == HAL_OK) {
        /* Clock out data bytes using dummy transfers */
        status = HAL_SPI_Receive(hspi, dest, length, 50);
    }
    CS_DESELECT();

    return status;
}
```

---

### 4.2.5 SPI Timing Calculations and Maximum Baud Rates

SPI clock frequency is derived directly from the host bus clock ($f_{\text{PCLK1}}$ or $f_{\text{PCLK2}}$) divided by a power-of-two prescaler:

$$f_{\text{SCK}} = \frac{f_{\text{PCLK}}}{\text{Prescaler}}, \quad \text{Prescaler} \in \{2, 4, 8, 16, 32, 64, 128, 256\}$$

#### Example: STM32F401 SPI1 ($f_{\text{PCLK2}} = 84\text{ MHz}$)
- $\text{Prescaler } 2 \implies f_{\text{SCK}} = 42\text{ MHz}$ (High-speed displays, external Flash)
- $\text{Prescaler } 8 \implies f_{\text{SCK}} = 10.5\text{ MHz}$ (Most IMUs and precision ADCs)
- $\text{Prescaler } 64 \implies f_{\text{SCK}} = 1.3125\text{ MHz}$ (Low-speed optical mice, long ribbon cables)

> [!CAUTION]
> **Signal Integrity on High-Speed SPI**: At frequencies above 10 MHz, SPI edges exhibit sub-nanosecond rise times. Without series termination resistors ($22\ \Omega \text{ to } 47\ \Omega$) placed near the transmitter, signal reflections and ringing on SCLK will create false clock edges, corrupting the transfer. Keep PCB traces under 10 cm and provide a continuous ground plane directly beneath SCLK and MOSI/MISO.

---

## 4.3 I²C (Inter-Integrated Circuit)

### 4.3.1 Operating Principle: The Open-Drain Wired-AND Bus

I²C is a synchronous, multi-master, multi-slave, half-duplex serial bus developed by Philips (NXP) that connects multiple integrated circuits using only **two bidirectional lines**:
1. **SDA (Serial Data)**: Carries data, addresses, and acknowledgment bits.
2. **SCL (Serial Clock)**: Synchronizes bit transfer; driven by the active Master (with optional clock stretching by Slaves).

#### Open-Drain Architecture
Unlike UART and SPI, which use push-pull output drivers (where transistors actively drive the line HIGH to $V_{DD}$ or LOW to GND), I²C outputs operate strictly in **open-drain** mode:
- To transmit a logic **LOW (0)**: The internal MOSFET turns ON, pulling the line to GND.
- To transmit a logic **HIGH (1)**: The internal MOSFET turns OFF, leaving the line floating. External **pull-up resistors ($R_p$)** pull the line voltage up to $V_{DD}$.

```text
+3.3V (VDD)
  │      │
 [Rp]   [Rp]
  │      │
  ├──────┼──────── SCL
  │  ┌───┼──────── SDA
  │  │   │  │
┌─┴──┴┐ ┌┴──┴─┐
│STM32│ │IMU  │  (All devices connect via Open-Drain NMOS drivers)
└─────┘ └─────┘
```

This creates a **Wired-AND** circuit: if even one device pulls the line LOW, the entire bus remains LOW. This physical property enables:
1. **Clock Stretching**: A slow slave can hold SCL LOW to halt the Master until its internal ADC finishes converting.
2. **Multi-Master Arbitration**: If two masters transmit simultaneously, the master asserting a '0' wins the bus without causing a short circuit, while the master attempting to send a '1' senses the mismatch and immediately yields.

---

### 4.3.2 Packet Structure: START, STOP, and ACK/NACK

Every transaction on an I²C bus follows a rigid packet structure clocked in 9-bit groups (8 bits data + 1 bit acknowledge).

```text
SDA: ───┐   ┌─── MSB ──────────────── LSB ───┐   ┌─── ACK/NACK ───┐   ┌───
        └───┤ D7 │ D6 │ D5 │ D4 │ D3 │ D2 │ D1 │ D0 ├───┤             ├───┘
SCL: ─┐     ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐     ┌───
      └─────┘ 1 └─┘ 2 └─┘ 3 └─┘ 4 └─┘ 5 └─┘ 6 └─┘ 7 └─┘ 8 └─┘ 9 └─────┘
      START                                                   STOP
```

1. **START Condition (S)**: A transition from HIGH to LOW on the SDA line while SCL remains HIGH. Signals all slaves to listen for an address.
2. **Address Byte (7-bit Address + R/W bit)**:
   - Bits `[7:1]`: 7-bit slave device address.
   - Bit `[0]`: Transfer direction (`0` = Master Write, `1` = Master Read).
3. **Acknowledge (ACK) / Negative Acknowledge (NACK)**:
   - During the 9th SCL clock pulse, the transmitter releases SDA (floating HIGH).
   - If a slave recognizes its address or successfully receives a byte, it pulls SDA **LOW** (**ACK**).
   - If the slave fails to pull SDA LOW, the line floats HIGH (**NACK**), indicating an error, an unpopulated device, or end of read.
4. **Data Bytes**: 8-bit payloads transferred MSB first, each followed by an ACK/NACK bit from the receiver.
5. **Repeated START Condition (Sr)**: A new START condition issued without first issuing a STOP condition. Used to switch from write to read mode atomically without releasing bus control.
6. **STOP Condition (P)**: A transition from LOW to HIGH on the SDA line while SCL remains HIGH. Returns the bus to the IDLE state.

---

### 4.3.3 The 7-Bit Address Left-Shift Requirement in STM32 HAL

One of the most frequent traps encountered by embedded engineers involves I²C device addressing in STM32 HAL.

Sensor datasheets specify the device address as a **7-bit hexadecimal value**. For example:
- **MPU6050 (AD0 pin LOW)**: Address = `0x68` (`0b1101000`)
- **SSD1306 OLED**: Address = `0x3C` (`0b0111100`)
- **BMP280 (SDO pin to GND)**: Address = `0x76` (`0b1110110`)

However, the hardware I²C shift register places the 7 address bits into bits `[7:1]` of the first transmitted byte, leaving bit `[0]` for the Read/Write flag:

```text
Transmitted Address Byte:
┌───┬───┬───┬───┬───┬───┬───┬─────┐
│ A6│ A5│ A4│ A3│ A2│ A1│ A0│ R/W │
└───┴───┴───┴───┴───┴───┴───┴─────┘
 Bit 7                         Bit 0
```

> [!WARNING]
> **The HAL 1-Bit Shift Rule**: The STM32 HAL functions (`HAL_I2C_Master_Transmit`, `HAL_I2C_Mem_Read`, etc.) require the `DevAddress` argument to be pre-shifted left by 1 bit:
> ```c
> #define MPU6050_7BIT_ADDR   0x68
> #define MPU6050_HAL_ADDR    (MPU6050_7BIT_ADDR << 1) /* 0xD0 */
> ```
> If you pass unshifted `0x68` to HAL, the hardware transmits `0x34` on the bus, and the sensor will never respond with an ACK!

---

### 4.3.4 Physical Layer Mathematics: Sizing Pull-Up Resistors ($R_p$)

Because open-drain pins cannot drive a line HIGH, the rising edge of both SCL and SDA is governed entirely by an RC charging curve formed by the pull-up resistor $R_p$ and the total parasitic bus capacitance $C_b$ (pin input capacitance + PCB trace capacitance + connector capacitance):

$$V(t) = V_{DD} \left(1 - e^{-\frac{t}{R_p C_b}}\right)$$

Under the NXP I²C specification (UM10204), the rise time $t_r$ is measured between the $30\%$ threshold ($0.3 V_{DD}$) and the $70\%$ threshold ($0.7 V_{DD}$). The mathematical time elapsed between these two voltages is:

$$t_r = R_p C_b \left( \ln\frac{1}{1 - 0.7} - \ln\frac{1}{1 - 0.3} \right) = R_p C_b \left( \ln(3.333) - \ln(1.428) \right) \approx 0.8473 \times R_p C_b$$

#### 1. Maximum Resistor Value ($R_{p,\max}$)
To guarantee that the signal rises fast enough to meet the maximum allowable rise time $t_{r,\max}$ at the chosen speed:

$$R_{p,\max} = \frac{t_{r,\max}}{0.8473 \times C_b}$$

- **Standard Mode (100 kHz)**: $t_{r,\max} = 1000\text{ ns}$. For $C_b = 100\text{ pF}$:
  $$R_{p,\max} = \frac{1000 \times 10^{-9}}{0.8473 \times 100 \times 10^{-12}} \approx 11.8\text{ k}\Omega$$
- **Fast Mode (400 kHz)**: $t_{r,\max} = 300\text{ ns}$. For $C_b = 100\text{ pF}$:
  $$R_{p,\max} = \frac{300 \times 10^{-9}}{0.8473 \times 100 \times 10^{-12}} \approx 3.54\text{ k}\Omega$$

#### 2. Minimum Resistor Value ($R_{p,\min}$)
When an STM32 pin drives LOW, the pull-up resistor conducts current into the output transistor. To prevent damaging the pin and ensure the low-level output voltage stays below $V_{OL,\max} = 0.4\text{ V}$ at the rated sink current $I_{OL} = 3\text{ mA}$:

$$R_{p,\min} = \frac{V_{DD} - V_{OL,\max}}{I_{OL}} = \frac{3.3\text{ V} - 0.4\text{ V}}{3\text{ mA}} \approx 967\ \Omega$$

**Summary Rule of Thumb for 3.3V Systems**:
- **$4.7\text{ k}\Omega$**: Universal standard for 100 kHz Standard Mode.
- **$2.2\text{ k}\Omega$**: Required for 400 kHz Fast Mode and buses with 3+ sensors.
- **Internal Weak Pull-ups ($40\text{ k}\Omega$)**: Completely inadequate! They yield rise times exceeding $3.5\ \mu\text{s}$, causing severe I²C clock distortion.

---

### 4.3.5 HAL Sensor Communication: `HAL_I2C_Mem_Read` vs. `HAL_I2C_Master_Receive`

Most digital sensors organize their internal configuration and data in 8-bit registers. Reading a sensor register requires a **write-then-read** sequence:
1. Master transmits START + Device Address (Write).
2. Master transmits the Target Register Pointer (e.g., `0x3B` for MPU6050 Accelerometer).
3. Master issues a **Repeated START** + Device Address (Read).
4. Slave clocks out register contents while Master acknowledges.
5. Master terminates with NACK + STOP.

STM32 HAL wraps this entire sequence into a single atomic function: `HAL_I2C_Mem_Read()`.

**Listing 4.5 — Efficient sensor register read and write operations**

```c
#define MPU6050_ADDR    (0x68 << 1)
#define REG_PWR_MGMT_1  0x6B
#define REG_ACCEL_XOUT  0x3B

/* Write a single 8-bit register */
HAL_StatusTypeDef Sensor_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, reg, 
                             I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

/* Read a continuous burst of 14 bytes (Accel, Temp, Gyro) in one I2C transaction */
HAL_StatusTypeDef Sensor_ReadBurst(I2C_HandleTypeDef *hi2c, uint8_t start_reg, 
                                  uint8_t *buffer, uint16_t len)
{
    return HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, start_reg, 
                            I2C_MEMADD_SIZE_8BIT, buffer, len, 100);
}
```

---

### 4.3.6 The Famous I²C Bus Lockup and 9-Clock Hardware Recovery

One of the most frustrating failure modes in embedded development occurs when an STM32 resets (due to a watchdog timeout, brownout, or debugger connection) in the middle of reading an I²C slave byte.

#### Why the Bus Locks Up
If the MCU resets while the slave is transmitting a data bit of value **`0`**, the slave's open-drain driver remains clamped to GND, holding the **SDA line permanently LOW**.
When the STM32 reboots, its I²C peripheral initializes and checks the bus state. Seeing SDA LOW, the STM32 hardware controller reports **`HAL_I2C_STATE_BUSY`** and refuses to transmit START conditions! The entire firmware halts permanently.

#### The 9-Clock Recovery Sequence
To free a jammed slave without power-cycling the board, the MCU must manually toggle the SCL clock up to 9 times to complete the slave's internal shift cycle, followed by a STOP condition.

**Listing 4.6 — I²C GPIO unjam / bus recovery procedure**

```c
void I2C_Bus_Clear(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                   GPIO_TypeDef *sda_port, uint16_t sda_pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. Configure SCL and SDA as GPIO open-drain outputs */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Pin = scl_pin;
    HAL_GPIO_Init(scl_port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = sda_pin;
    HAL_GPIO_Init(sda_port, &GPIO_InitStruct);

    /* 2. Release SDA and check if line is clamped LOW */
    HAL_GPIO_WritePin(sda_port, sda_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
    HAL_Delay(1);

    /* If SDA is clamped LOW by a confused slave, clock SCL up to 9 times */
    if (HAL_GPIO_ReadPin(sda_port, sda_pin) == GPIO_PIN_RESET) {
        for (int i = 0; i < 9; i++) {
            HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 200; d++); /* ~5 µs delay */
            HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
            for (volatile int d = 0; d < 200; d++);

            /* Check if slave released SDA */
            if (HAL_GPIO_ReadPin(sda_port, sda_pin) == GPIO_PIN_SET) {
                break;
            }
        }
    }

    /* 3. Generate a STOP condition: SDA transitions LOW to HIGH while SCL is HIGH */
    HAL_GPIO_WritePin(sda_port, sda_pin, GPIO_PIN_RESET);
    for (volatile int d = 0; d < 200; d++);
    HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
    for (volatile int d = 0; d < 200; d++);
    HAL_GPIO_WritePin(sda_port, sda_pin, GPIO_PIN_SET);
    for (volatile int d = 0; d < 200; d++);

    /* 4. Pins will be reconfigured as Alternate Function by MX_I2C1_Init() */
}
```

**Table 4.3 — I²C debugging and symptom diagnostic matrix**

| Symptom | Primary Root Cause | Corrective Engineering Action |
|---|---|---|
| `HAL_BUS_ERROR` / NACK on first byte | Incorrect slave address; forgotten 1-bit left shift | Shift 7-bit address left: `(ADDR << 1)` |
| `HAL_BUSY` immediately upon boot | SDA clamped LOW by a hung slave from previous reset | Execute 9-clock bit-bang unjam sequence before `MX_I2C_Init()` |
| Intermittent NACKs at 400 kHz | Pull-up resistors too weak ($10\text{ k}\Omega$); rounded edges | Replace pull-ups with $2.2\text{ k}\Omega$; inspect SCL on oscilloscope |
| Driver hangs inside `HAL_I2C_Mem_Read` | Slave asserting continuous Clock Stretching | Pass strict timeout parameter (e.g., `100 ms`) instead of `HAL_MAX_DELAY` |
| `WHO_AM_I` reads `0x00` or `0xFF` | Wrong register address, sensor in reset, or $V_{DD}$ unpowered | Verify power rails with DMM; verify reset pin logic state |

---

## 4.4 Comprehensive Protocol Comparison and Selection Matrix

Choosing the correct serial protocol is an architectural decision that directly influences circuit board complexity, electromagnetic emissions, MCU pin utilization, software complexity, and system throughput.

**Table 4.4 — Comprehensive technical comparison of serial protocols**

| Technical Parameter | UART / USART | SPI | I²C |
|---|---|---|---|
| **Topology** | Point-to-point (2 devices) | Single Master, Multi-Slave | Multi-Master, Multi-Slave |
| **Number of Wires** | **2** (TX, RX) + GND | **3 + N** (SCLK, MOSI, MISO + CS per slave) | **2** (SDA, SCL) + GND |
| **Duplex Capability** | Full-Duplex | Full-Duplex | Half-Duplex |
| **Clock Synchronization** | Asynchronous (Agreed Baud) | Synchronous (Dedicated SCLK) | Synchronous (Dedicated SCL) |
| **Maximum Data Rate** | Typical: 115.2 kbps – 921.6 kbps<br>Max: 10–27 Mbps (High-end) | Typical: 10 Mbps – 40 Mbps<br>Max: 80+ Mbps | Standard: 100 kbps<br>Fast: 400 kbps<br>Fast+: 1 Mbps |
| **Maximum Distance** | Single board: 2–3 m<br>(RS-232/485: up to 1200 m) | Very Short: < 30 cm on PCB | Short: < 1 m (governed by 400 pF bus capacitance) |
| **Hardware Addressing** | None (Software framing required) | None (Hardware CS pin asserts slave) | Built-in 7-bit or 10-bit software addressing |
| **Flow Control** | Optional RTS/CTS lines | None | Built-in Clock Stretching and ACK/NACK |
| **Bus Contention / Safety** | Driver collision if outputs shorted | Driver collision if multiple CS asserted | Safe (Open-drain Wired-AND logic) |
| **Typical Transmit Overhead** | 20% (1 Start + 1 Stop bit per 8 bits) | **0%** (Pure raw streaming bytes) | ~20–30% (START, Address, ACK per byte) |
| **Primary Use Cases** | Debug console, GPS, Cellular modems | Flash memory, Displays, High-rate IMUs | Temp sensors, RTC, EEPROM, Configuration |

---

### 4.4.1 Industrial Protocols Overview: CAN, RS-485, USB, and Ethernet

For applications demanding longer physical transmission lines, high electrical noise immunity, or multi-megabyte throughput, standard UART/SPI/I²C protocols are superseded by industrial standards:

1. **CAN Bus (Controller Area Network)**:
   - *Physical Layer*: Differential balanced two-wire bus (CAN_H and CAN_L) terminated with $120\ \Omega$ at both ends.
   - *Key Advantage*: Built-in non-destructive bitwise arbitration, automatic CRC error detection, hardware message filtering, and exceptional immunity to ground offsets and motor noise.
   - *Use*: Robotics drivetrain, automotive ECUs, UAV avionics.
2. **RS-485**:
   - *Physical Layer*: Differential UART signaling over twisted-pair wiring ($A$ and $B$).
   - *Key Advantage*: Extends standard UART streams up to 1200 meters and supports up to 32 transceivers in multi-drop configurations.
   - *Use*: Industrial Modbus networks, PTZ surveillance cameras, long-range solar sensors.
3. **USB (Universal Serial Bus)**:
   - *Physical Layer*: Half-duplex differential pair (D+, D-) operating at Full Speed (12 Mbps) or High Speed (480 Mbps).
   - *Key Advantage*: Native plug-and-play PC connectivity, packetized enumeration, virtual COM port (VCP).
   - *Use*: Firmware flashing, host computer high-speed data acquisition.
4. **Ethernet (10/100/1000 Mbps)**:
   - *Physical Layer*: RMII/MII interface to external PHY chip; transformer-isolated twisted pairs.
   - *Key Advantage*: Standard TCP/IP networking, global routability, extremely high bandwidth.
   - *Use*: Industrial IoT gateways, vision processing nodes.

---

### 4.4.2 Practical Engineering Selection Scenarios

To decide which protocol to use when designing an STM32-based robotic system:

#### Scenario A: High-Rate 6-Axis IMU (MPU6050 / ICM-20689)
- *Bandwidth Requirement*: Reading 14 bytes (Accel, Gyro, Temp) at 1000 Hz ($14\text{ kB/s}$).
- *Evaluation*:
  - **I²C at 100 kHz**: Each 14-byte read requires 155 clock cycles $\approx 1.55\text{ ms}$. Cannot achieve 1000 Hz!
  - **I²C at 400 kHz**: 155 clock cycles $\approx 388\ \mu\text{s}$. Consumes 39% of the CPU bus bandwidth.
  - **SPI at 10 MHz**: 15 bytes clocked out in $12\ \mu\text{s}$. Negligible bus utilization (< 1.2%).
- *Decision*: **Use SPI** if available on the sensor breakout. Use I²C only if pin constraints force it.

#### Scenario B: Debugging Telemetry Console to PC
- *Requirement*: Simple, streaming textual diagnostics to terminal software (PuTTY / TeraTerm).
- *Decision*: **Use UART (115,200 or 921,600 baud)** connected to the ST-LINK Virtual COM Port (VCP). Universal, non-intrusive, requires no dedicated PC drivers.

#### Scenario C: External 128 Mbit Flash Memory (W25Q128)
- *Requirement*: Storing graphics assets and logging large flight telemetry arrays at high speed.
- *Decision*: **Use SPI (or Quad-SPI)**. Quad-SPI allows 4 data bits per clock, achieving up to 40 MB/sec throughput.

#### Scenario D: Multiple Environmental Sensors (Temperature, Humidity, Pressure, RTC)
- *Requirement*: Sampling 4 different low-bandwidth sensors every 1 second across a small robot chassis.
- *Decision*: **Use I²C**. Connects all 4 devices on the exact same two pins (`SDA` and `SCL`), conserving vital microcontroller pins for motor PWM and encoder timers.

---

### 4.4.3 Architectural Decision Flowchart

```text
Do you need to connect to a PC / Terminal / Modem?
 │
 ├──► YES ──► Does it require standard networking (TCP/IP)?
 │             ├──► YES ──► Ethernet (RMII)
 │             └──► NO  ──► Is USB available? 
 │                           ├──► YES ──► USB VCP / HID
 │                           └──► NO  ──► UART (with RS-232/VCP)
 │
 └──► NO (Inter-chip or Peripheral Connection)
       │
       ├──► Is transmission distance > 3 meters in a noisy environment?
       │     ├──► Multi-drop automotive/robotics ──► CAN Bus
       │     └──► Master-slave telemetry ──────────► RS-485
       │
       └──► On-Board Peripheral (< 30 cm)
             │
             ├──► High Speed (> 1 Mbps), Displays, Flash, or Fast IMUs?
             │     └──► Use SPI (Dedicated CS per slave)
             │
             └──► Multiple low-speed sensors (< 400 kbps), low pin count?
                   └──► Use I²C (Shared SDA/SCL, 2.2kΩ pull-ups)
```
