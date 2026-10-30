import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DIAGRAMS_DIR = os.path.join(SCRIPT_DIR, "diagrams")
os.makedirs(DIAGRAMS_DIR, exist_ok=True)

OUTPUT_DOCX_PATH = os.path.join(SCRIPT_DIR, "STM32_Complete_Professional_Reference.docx")

NAVY = "#0F2537"
BLUE = "#1E4E79"
TEAL = "#008080"
CYAN = "#00A896"
SLATE = "#334155"
LIGHT_BG = "#F8FAFC"
ICE = "#E0F2FE"
MINT = "#CCFBF1"
AMBER = "#FEF3C7"
ORANGE = "#D97706"
RED_BG = "#FEE2E2"
RED = "#DC2626"
BORDER = "#94A3B8"

def draw_box(ax, x, y, w, h, text, bg=ICE, edge=BLUE, text_color=NAVY, fontsize=9, weight='bold', subtext=None, subsize=7.5):
    box = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02,rounding_size=0.08",
                         facecolor=bg, edgecolor=edge, linewidth=1.5)
    ax.add_patch(box)
    if subtext:
        ax.text(x + w/2, y + h*0.66, text, ha='center', va='center', color=text_color,
                fontsize=fontsize, fontweight=weight, family='sans-serif')
        ax.text(x + w/2, y + h*0.30, subtext, ha='center', va='center', color=SLATE,
                fontsize=subsize, family='sans-serif')
    else:
        ax.text(x + w/2, y + h/2, text, ha='center', va='center', color=text_color,
                fontsize=fontsize, fontweight=weight, family='sans-serif')

def draw_arrow(ax, x1, y1, x2, y2, color=NAVY, label=None, label_pos=0.5, fontsize=8, style="-|>", scale=13):
    arrow = FancyArrowPatch((x1, y1), (x2, y2), arrowstyle=style, mutation_scale=scale,
                            color=color, linewidth=1.5)
    ax.add_patch(arrow)
    if label:
        lx = x1 + (x2 - x1)*label_pos
        ly = y1 + (y2 - y1)*label_pos
        ax.text(lx, ly + 0.12, label, ha='center', va='bottom', fontsize=fontsize,
                color=color, fontweight='bold',
                bbox=dict(boxstyle='round,pad=0.12', facecolor='white', edgecolor='none', alpha=0.88))

# ------------------------------------------------------------------------------
# DIAGRAM GENERATION (1 to 6)
# ------------------------------------------------------------------------------
print("Generating high-resolution technical architecture diagrams...")

# DIAGRAM 1: 10-Layer Signal Chain & Multi-AHB Bus Matrix
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 6.2), dpi=220)
fig.patch.set_facecolor('white')

ax1.set_xlim(0, 10)
ax1.set_ylim(0, 11.5)
ax1.axis('off')
ax1.set_title("A. 10-Layer Embedded Signal & Ownership Chain", fontsize=11, fontweight='bold', color=NAVY, pad=10)

layers = [
    ("1. Physical World", "Temp, Pressure, Motion, Light, Acoustic Waves", "#E0F2FE", BLUE),
    ("2. Electrical Front-End", "Voltage Divider, RC Anti-Alias Filter, Pull-Ups, Level Shifter", "#E0F2FE", BLUE),
    ("3. MCU Pin & Pad Mux", "GPIO Mode, Schmitt Trigger, Analog Switch, Alternate Function", "#CCFBF1", TEAL),
    ("4. Hardware Peripheral", "SAR ADC, USART, SPI, I2C, TIM Capture/Compare, EXTI", "#CCFBF1", TEAL),
    ("5. Data Movement Engine", "Polling Read, ISR Byte Copy, or DMA Channel/Stream Transfer", "#FEF3C7", ORANGE),
    ("6. Event Notification", "Status Flag, NVIC Interrupt, DMA Half/Full Callback, RTOS Queue", "#FEF3C7", ORANGE),
    ("7. Buffer Ownership", "Ping-Pong Half Lock, Ring Buffer Indices, Mutex Protection", "#FCE7F3", "#9D174D"),
    ("8. Driver & DSP Layer", "WHO_AM_I Check, Scaling to SI Units, Moving Average / EMA Filter", "#EDE9FE", "#5B21B6"),
    ("9. Application Policy", "State Machine, Thresholds, Fault Recovery, Control Loop", "#DCFCE7", "#166534"),
    ("10. Output & Telemetry", "PWM Actuator Drive, Framed UART Telemetry, CAN/SPI Display", "#DCFCE7", "#166534")
]

for idx, (title, desc, bg, edge) in enumerate(layers):
    y_pos = 10.2 - idx * 1.02
    draw_box(ax1, 0.6, y_pos, 8.8, 0.78, title, bg=bg, edge=edge, fontsize=8.5, subtext=desc, subsize=7.2)
    if idx < len(layers) - 1:
        draw_arrow(ax1, 5.0, y_pos, 5.0, y_pos - 0.24, color=SLATE, scale=10)

ax2.set_xlim(0, 10)
ax2.set_ylim(0, 11.5)
ax2.axis('off')
ax2.set_title("B. STM32 Internal Architecture & Multi-AHB Bus Matrix", fontsize=11, fontweight='bold', color=NAVY, pad=10)

draw_box(ax2, 0.5, 9.5, 4.0, 1.3, "ARM Cortex-M Core", bg="#DBEAFE", edge=NAVY, fontsize=9.5,
         subtext="CPU + FPU + NVIC + SysTick")
draw_box(ax2, 5.3, 9.5, 4.2, 1.3, "DMA Controllers", bg="#FEF3C7", edge=ORANGE, fontsize=9.5,
         subtext="DMA1 / DMA2 / DMAMUX (Bus Master)")

draw_box(ax2, 0.5, 7.3, 9.0, 1.2, "Multi-Layer AHB Bus Matrix & Interconnect", bg="#F1F5F9", edge=SLATE, fontsize=9.5,
         subtext="Arbitrates concurrent CPU & DMA access to SRAM, Flash, and Peripherals")

draw_arrow(ax2, 2.5, 9.5, 2.5, 8.5, color=NAVY, style="<->")
draw_arrow(ax2, 7.4, 9.5, 7.4, 8.5, color=ORANGE, style="<->")

draw_box(ax2, 0.5, 5.3, 2.7, 1.2, "Flash Memory", bg="#E0E7FF", edge="#3730A3", fontsize=8.5,
         subtext="Code + Constants\n(Wait States / Latency)")
draw_box(ax2, 3.6, 5.3, 2.7, 1.2, "SRAM1 / SRAM2", bg="#E0E7FF", edge="#3730A3", fontsize=8.5,
         subtext="Stacks, Heap, Buffers\n(DMA Accessible)")
draw_box(ax2, 6.7, 5.3, 2.8, 1.2, "AHB Peripherals", bg="#CCFBF1", edge=TEAL, fontsize=8.5,
         subtext="GPIO Ports, RCC, CRC\n(ADC on AHB: newer families)")

draw_arrow(ax2, 1.85, 7.3, 1.85, 6.5, color=SLATE, style="<->")
draw_arrow(ax2, 4.95, 7.3, 4.95, 6.5, color=SLATE, style="<->")
draw_arrow(ax2, 8.1, 7.3, 8.1, 6.5, color=TEAL, style="<->")

draw_box(ax2, 0.5, 3.1, 4.2, 1.3, "APB1 Bridge (Low-Speed Bus)", bg="#F0FDF4", edge="#15803D", fontsize=8.5,
         subtext="PCLK1: TIM2-TIM7, USART2/3,\nI2C1/2, SPI2/3, CAN, PWR, IWDG")
draw_box(ax2, 5.3, 3.1, 4.2, 1.3, "APB2 Bridge (High-Speed Bus)", bg="#F0FDF4", edge="#15803D", fontsize=8.5,
         subtext="PCLK2: TIM1/TIM8, USART1,\nSPI1, ADC1/2, EXTI, SYSCFG")

draw_arrow(ax2, 2.6, 5.3, 2.6, 4.4, color=SLATE, style="<->")
draw_arrow(ax2, 7.4, 5.3, 7.4, 4.4, color=SLATE, style="<->")

draw_box(ax2, 0.5, 0.8, 9.0, 1.5, "RCC (Reset & Clock Control) & Clock Tree Distribution", bg="#FFF1F2", edge="#BE123C", fontsize=9,
         subtext="HSI / HSE / PLL -> SYSCLK -> HCLK (AHB) -> PCLK1 / PCLK2 (APB)\nIncludes APB Timer x2 Multiplier & Dedicated Peripheral Kernel Clocks", subsize=7.2)

plt.tight_layout()
plt.savefig(os.path.join(DIAGRAMS_DIR, "fig1_system_architecture.png"), dpi=220)
plt.close()

# DIAGRAM 2: CubeMX/CubeIDE Workflow & HAL-to-Register Mapping
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5.2), dpi=220)
fig.patch.set_facecolor('white')

ax1.set_xlim(0, 10)
ax1.set_ylim(0, 10)
ax1.axis('off')
ax1.set_title("A. STM32CubeMX (.ioc) to Firmware Lifecycle", fontsize=11, fontweight='bold', color=NAVY)

steps = [
    ("1. Target & .ioc Setup", "Select exact MCU part, pinout, AF, Clock Tree, NVIC & DMA in CubeMX", 8.3, ICE, BLUE),
    ("2. Code Generation", "Generates main.c, *_hal_msp.c, *_it.c, SystemClock_Config()", 6.5, MINT, TEAL),
    ("3. Modular User Code", "Write app.c & sensor drivers inside USER CODE blocks or separate modules", 4.7, AMBER, ORANGE),
    ("4. GNU Arm Build & Link", "arm-none-eabi-gcc compiles & links via STM32xxxx_FLASH.ld", 2.9, "#EDE9FE", "#5B21B6"),
    ("5. SWD Flash & Verification", "Flash via ST-LINK/J-Link; verify with Breakpoints, SFRs & Scope", 1.1, "#DCFCE7", "#166534")
]
for title, sub, y, bg, edge in steps:
    draw_box(ax1, 0.8, y, 8.4, 1.3, title, bg=bg, edge=edge, fontsize=9, subtext=sub, subsize=7.5)
for i in range(len(steps)-1):
    draw_arrow(ax1, 5.0, steps[i][2], 5.0, steps[i+1][2]+1.3, color=NAVY)

ax2.set_xlim(0, 10)
ax2.set_ylim(0, 10)
ax2.axis('off')
ax2.set_title("B. Software Abstraction Stack (Application to Silicon)", fontsize=11, fontweight='bold', color=NAVY)

stack = [
    ("Application & RTOS Tasks (app.c)", "High-level control logic, state machines, telemetry formatting", 8.2, "#DCFCE7", "#166534"),
    ("Reusable Sensor Driver (mpu6050.c)", "Device-specific registers, scaling to physical units, error status", 6.5, "#EDE9FE", "#5B21B6"),
    ("STM32 HAL / LL Driver Layer", "HAL_I2C_Mem_Read(), HAL_GPIO_WritePin() | LL_GPIO_SetOutputPin()", 4.8, ICE, BLUE),
    ("CMSIS Memory-Mapped Structs", "GPIOC->BSRR = (1U << 29); | I2C1->CR1 |= I2C_CR1_PE;", 3.1, AMBER, ORANGE),
    ("Physical Peripheral Silicon Registers", "Flip-flops, shift registers, prescalers, SAR ADC & I/O transistors", 1.4, "#FFE4E6", "#BE123C")
]
for title, sub, y, bg, edge in stack:
    draw_box(ax2, 0.7, y, 8.6, 1.25, title, bg=bg, edge=edge, fontsize=9, subtext=sub, subsize=7.5)
for i in range(len(stack)-1):
    draw_arrow(ax2, 5.0, stack[i][2], 5.0, stack[i+1][2]+1.25, color=SLATE, style="<->")

plt.tight_layout()
plt.savefig(os.path.join(DIAGRAMS_DIR, "fig2_workflow_and_hal.png"), dpi=220)
plt.close()

# DIAGRAM 3: Polling vs Interrupt vs Circular DMA Timeline
fig, ax = plt.subplots(figsize=(13, 5.5), dpi=220)
fig.patch.set_facecolor('white')
ax.set_xlim(0, 13)
ax.set_ylim(0, 9.5)
ax.axis('off')
ax.set_title("Execution Paradigms: Polling vs. Interrupt-Driven vs. Circular DMA (Ping-Pong Buffer)", fontsize=12, fontweight='bold', color=NAVY, pad=10)

# Band 1: Polling
ax.text(0.2, 8.2, "1. Blocking Polling\n(CPU 100% Tied Up)", fontsize=9.5, fontweight='bold', color=RED, va='center')
for i in range(4):
    x0 = 2.6 + i*2.5
    draw_box(ax, x0, 7.6, 1.6, 1.1, "CPU Spins / Waits", bg=RED_BG, edge=RED, fontsize=8, subtext="HAL_ADC_PollForConv()")
    draw_box(ax, x0+1.65, 7.6, 0.75, 1.1, "Read", bg=ICE, edge=BLUE, fontsize=8, subtext="Process")

# Band 2: Interrupt
ax.text(0.2, 5.2, "2. Interrupt (ISR)\n(Per-Item Context Switch)", fontsize=9.5, fontweight='bold', color=ORANGE, va='center')
for i in range(4):
    x0 = 2.6 + i*2.5
    draw_box(ax, x0, 4.6, 1.7, 1.1, "CPU Useful Work", bg="#DCFCE7", edge="#166534", fontsize=8, subtext="Main Loop / RTOS Task")
    draw_box(ax, x0+1.75, 4.6, 0.65, 1.1, "ISR", bg=AMBER, edge=ORANGE, fontsize=8, subtext="Copy 1")

# Band 3: Circular DMA
ax.text(0.2, 2.0, "3. Timer + ADC + DMA\n(Zero Per-Sample CPU Load)", fontsize=9.5, fontweight='bold', color=TEAL, va='center')
draw_box(ax, 2.6, 2.3, 4.8, 0.95, "DMA Fills Lower Half: buf[0 .. N/2-1]", bg=MINT, edge=TEAL, fontsize=8.5,
         subtext="Hardware Triggered at Exact Sample Period Ts = 1/Fs")
draw_box(ax, 7.5, 2.3, 4.8, 0.95, "DMA Fills Upper Half: buf[N/2 .. N-1]", bg=MINT, edge=TEAL, fontsize=8.5,
         subtext="DMA Wraps Around Automatically in Circular Mode")

draw_box(ax, 2.6, 0.9, 4.4, 1.1, "CPU Free / Idle / Other Tasks", bg="#DCFCE7", edge="#166534", fontsize=8.5)
draw_box(ax, 7.05, 0.9, 0.4, 1.1, "HT", bg=AMBER, edge=ORANGE, fontsize=7.5)
draw_box(ax, 7.5, 0.9, 4.3, 1.1, "CPU Processes Lower Half buf[0..N/2-1]", bg=ICE, edge=BLUE, fontsize=8.5,
         subtext="Must finish before TC deadline: T_half = N / (2 * Fs)")
draw_box(ax, 11.85, 0.9, 0.45, 1.1, "TC", bg=AMBER, edge=ORANGE, fontsize=7.5)

draw_arrow(ax, 2.6, 0.4, 12.4, 0.4, color=NAVY, label="Time (t)  --->   Deterministic Hardware Sampling with Ping-Pong Buffer Ownership", label_pos=0.5, fontsize=8.5)

plt.tight_layout()
plt.savefig(os.path.join(DIAGRAMS_DIR, "fig3_polling_irq_dma.png"), dpi=220)
plt.close()

# DIAGRAM 4: Timer Architecture & Timer-Triggered ADC + DMA Pipeline
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(13, 6.2), dpi=220)
fig.patch.set_facecolor('white')

ax1.set_xlim(0, 13)
ax1.set_ylim(0, 4.2)
ax1.axis('off')
ax1.set_title("A. STM32 Hardware Timer Core Architecture (Prescaler, Counter, Auto-Reload & Capture/Compare)", fontsize=11, fontweight='bold', color=NAVY)

draw_box(ax1, 0.3, 1.4, 2.1, 1.6, "Timer Kernel Clock", bg="#FFF1F2", edge="#BE123C", fontsize=8.5,
         subtext="f_TIM (e.g., 72 MHz)\nCheck APB x2 Rule!")
draw_box(ax1, 2.9, 1.4, 2.3, 1.6, "16-bit Prescaler (PSC)", bg=ICE, edge=BLUE, fontsize=8.5,
         subtext="Divides by (PSC + 1)\nf_cnt = f_TIM / (PSC+1)")
draw_box(ax1, 5.7, 1.4, 2.4, 1.6, "Counter Register (CNT)", bg=MINT, edge=TEAL, fontsize=8.5,
         subtext="Counts 0 .. ARR\n(16-bit or 32-bit)")
draw_box(ax1, 8.6, 2.3, 2.0, 1.3, "Auto-Reload (ARR)", bg=AMBER, edge=ORANGE, fontsize=8.5,
         subtext="Period = ARR + 1\nUpdate Event (UEV)")
draw_box(ax1, 8.6, 0.5, 2.0, 1.3, "Capture/Compare (CCR)", bg="#EDE9FE", edge="#5B21B6", fontsize=8.5,
         subtext="PWM Duty ≈ CCR/(ARR+1)\nor Input Edge Latch")
draw_box(ax1, 11.0, 1.4, 1.8, 1.6, "Hardware Outputs", bg="#DCFCE7", edge="#166534", fontsize=8.5,
         subtext="1. TRGO -> ADC\n2. PWM Pin\n3. TIM_IRQn")

draw_arrow(ax1, 2.4, 2.2, 2.9, 2.2, color=NAVY)
draw_arrow(ax1, 5.2, 2.2, 5.7, 2.2, color=NAVY)
draw_arrow(ax1, 8.1, 2.6, 8.6, 2.8, color=ORANGE)
draw_arrow(ax1, 8.1, 1.8, 8.6, 1.2, color="#5B21B6")
draw_arrow(ax1, 10.6, 2.8, 11.0, 2.4, color=NAVY)
draw_arrow(ax1, 10.6, 1.2, 11.0, 1.9, color=NAVY)

ax2.set_xlim(0, 13)
ax2.set_ylim(0, 4.2)
ax2.axis('off')
ax2.set_title("B. Deterministic Real-Time Acquisition Pipeline: Timer TRGO -> SAR ADC -> Circular DMA -> DSP Task", fontsize=11, fontweight='bold', color=NAVY)

stages = [
    ("1. Hardware Timer", "Generates TRGO on\nUpdate Event at Fs", 0.3, ICE, BLUE),
    ("2. Analog Front-End", "Sensor + RC Filter\nRs settled in T_samp", 2.4, "#F1F5F9", SLATE),
    ("3. SAR ADC Engine", "Samples Vin -> 12-bit\nCode in ADC_DR", 4.5, MINT, TEAL),
    ("4. DMA Controller", "Moves ADC_DR to RAM\nCircular Half-Word", 6.6, AMBER, ORANGE),
    ("5. Ping-Pong RAM", "buf[0..N/2-1] &\nbuf[N/2..N-1]", 8.7, "#EDE9FE", "#5B21B6"),
    ("6. App / DSP Task", "Scale to SI Units,\nMA/EMA Filter, UART", 10.8, "#DCFCE7", "#166534")
]
for title, sub, x, bg, edge in stages:
    draw_box(ax2, x, 1.2, 1.85, 1.8, title, bg=bg, edge=edge, fontsize=8.5, subtext=sub, subsize=7.2)
for i in range(len(stages)-1):
    draw_arrow(ax2, stages[i][2]+1.85, 2.1, stages[i+1][2], 2.1, color=NAVY)

plt.tight_layout()
plt.savefig(os.path.join(DIAGRAMS_DIR, "fig4_timer_adc_dma.png"), dpi=220)
plt.close()

# DIAGRAM 5: UART, SPI, and I2C Protocol Topologies & Frames
fig, axes = plt.subplots(1, 3, figsize=(13.5, 4.8), dpi=220)
fig.patch.set_facecolor('white')

# Panel 1: UART
ax = axes[0]
ax.set_xlim(0, 10)
ax.set_ylim(0, 10)
ax.axis('off')
ax.set_title("A. UART (Asynchronous Point-to-Point)", fontsize=10, fontweight='bold', color=NAVY)
draw_box(ax, 0.5, 6.2, 3.6, 2.2, "STM32 USARTx", bg=ICE, edge=BLUE, fontsize=9, subtext="TX\nRX\nGND")
draw_box(ax, 5.9, 6.2, 3.6, 2.2, "Peer Device / VCP", bg=MINT, edge=TEAL, fontsize=9, subtext="RX\nTX\nGND")
draw_arrow(ax, 4.1, 7.6, 5.9, 7.6, color=NAVY, label="TX -> RX", fontsize=7.5)
draw_arrow(ax, 5.9, 6.9, 4.1, 6.9, color=TEAL, label="RX <- TX", fontsize=7.5)
draw_arrow(ax, 4.1, 6.3, 5.9, 6.3, color=SLATE, style="-")
draw_box(ax, 0.5, 1.2, 9.0, 3.8, "8-N-1 Frame Format (10 Bits/Byte)", bg="#F8FAFC", edge=SLATE, fontsize=8.5,
         subtext="Idle(1) | Start(0) | D0 D1 D2 D3 D4 D5 D6 D7 | Stop(1)\nNo shared clock; Baud error must be < 2%\nMax Payload ≈ Baud / 10 bytes/sec")

# Panel 2: SPI
ax = axes[1]
ax.set_xlim(0, 10)
ax.set_ylim(0, 10)
ax.axis('off')
ax.set_title("B. SPI (Synchronous Full-Duplex Shift)", fontsize=10, fontweight='bold', color=NAVY)
draw_box(ax, 0.5, 5.8, 3.6, 2.8, "STM32 SPI Master", bg=ICE, edge=BLUE, fontsize=9, subtext="SCK\nMOSI\nMISO\nGPIO CS")
draw_box(ax, 5.9, 5.8, 3.6, 2.8, "SPI Sensor / Flash", bg=AMBER, edge=ORANGE, fontsize=9, subtext="SCK\nSDI\nSDO\nCS (Active Low)")
draw_arrow(ax, 4.1, 8.0, 5.9, 8.0, color=NAVY, label="SCK", fontsize=7)
draw_arrow(ax, 4.1, 7.3, 5.9, 7.3, color=BLUE, label="MOSI", fontsize=7)
draw_arrow(ax, 5.9, 6.6, 4.1, 6.6, color=ORANGE, label="MISO", fontsize=7)
draw_arrow(ax, 4.1, 5.9, 5.9, 5.9, color=RED, label="CS", fontsize=7)
draw_box(ax, 0.5, 1.2, 9.0, 3.8, "SPI Modes 0-3 (CPOL & CPHA)", bg="#F8FAFC", edge=SLATE, fontsize=8.5,
         subtext="Mode 0 (0,0): Idle Low, Sample 1st Rising Edge\nMode 3 (1,1): Idle High, Sample 2nd Rising Edge\nFull-duplex: Master sends dummy byte (0x00) to clock read")

# Panel 3: I2C
ax = axes[2]
ax.set_xlim(0, 10)
ax.set_ylim(0, 10)
ax.axis('off')
ax.set_title("C. I²C (Shared Open-Drain Addressed Bus)", fontsize=10, fontweight='bold', color=NAVY)
draw_box(ax, 0.5, 8.2, 9.0, 1.1, "3.3 V rail + external pull-ups (Rp ≈ 2.2-4.7 kΩ)", bg="#FFF1F2", edge="#BE123C", fontsize=8)
draw_box(ax, 0.5, 5.4, 2.8, 2.0, "STM32 Master", bg=ICE, edge=BLUE, fontsize=8.5, subtext="SCL / SDA\n(Open-Drain)")
draw_box(ax, 3.7, 5.4, 2.8, 2.0, "MPU6050 IMU", bg=MINT, edge=TEAL, fontsize=8.5, subtext="7-bit: 0x68\nHAL: 0x68<<1")
draw_box(ax, 6.9, 5.4, 2.6, 2.0, "Temp Sensor", bg=MINT, edge=TEAL, fontsize=8.5, subtext="7-bit: 0x48\nHAL: 0x48<<1")
draw_box(ax, 0.5, 1.2, 9.0, 3.6, "I²C Register Read Transaction", bg="#F8FAFC", edge=SLATE, fontsize=8.5,
         subtext="START | Addr7+W | ACK | RegAddr | ACK |\nRESTART | Addr7+R | ACK | Data[0..k] | NACK | STOP\nRise Time: tr ≈ 0.847 * Rp * Cbus (Max 300ns @ 400kHz)")

plt.tight_layout()
plt.savefig(os.path.join(DIAGRAMS_DIR, "fig5_serial_protocols.png"), dpi=220)
plt.close()

# DIAGRAM 6: Reusable Sensor Driver & FreeRTOS Task Architecture
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5.4), dpi=220)
fig.patch.set_facecolor('white')

ax1.set_xlim(0, 10)
ax1.set_ylim(0, 10)
ax1.axis('off')
ax1.set_title("A. 4-Layer Reusable Sensor Driver Architecture", fontsize=10.5, fontweight='bold', color=NAVY)

layers_drv = [
    ("Application Layer (app.c)", "Calls MPU6050_ReadSensors(&dev, &data); Evaluates pitch/roll/alarms\nZero knowledge of register hex addresses or I2C shift rules", 7.8, "#DCFCE7", "#166534"),
    ("Sensor Driver API (mpu6050.h / .c)", "Owns Register Map (WHO_AM_I=0x75, PWR_MGMT_1=0x6B),\nTwo's Complement Sign-Extension, LSB Scaling, Status Enum", 5.3, "#EDE9FE", "#5B21B6"),
    ("Bus / Transport Interface", "Injected I2C_HandleTypeDef* or Function-Pointer IO Struct\nEnforces bounded timeouts & Mutex locking on shared bus", 2.8, ICE, BLUE),
    ("CubeMX Hardware & HAL MSP", "MX_I2C1_Init(), GPIO Open-Drain AF4, Clock Tree & NVIC", 0.3, AMBER, ORANGE)
]
for title, sub, y, bg, edge in layers_drv:
    draw_box(ax1, 0.6, y, 8.8, 1.65, title, bg=bg, edge=edge, fontsize=8.5, subtext=sub, subsize=7.3)
for i in range(len(layers_drv)-1):
    draw_arrow(ax1, 5.0, layers_drv[i][2], 5.0, layers_drv[i+1][2]+1.65, color=NAVY, style="<->", scale=11)

ax2.set_xlim(0, 10)
ax2.set_ylim(0, 10)
ax2.axis('off')
ax2.set_title("B. FreeRTOS Real-Time Sensor Monitoring Architecture", fontsize=10.5, fontweight='bold', color=NAVY)

draw_box(ax2, 0.5, 8.0, 4.2, 1.4, "Hardware Timer + ADC + DMA", bg=MINT, edge=TEAL, fontsize=8.5,
         subtext="DMA Half/Full ISR\nvTaskNotifyGiveFromISR()")
draw_box(ax2, 5.3, 8.0, 4.2, 1.4, "I²C Digital Sensors (MPU6050)", bg=ICE, edge=BLUE, fontsize=8.5,
         subtext="Protected by I2C Bus Mutex\nBounded Timeout (10-50 ms)")

draw_box(ax2, 0.5, 5.5, 9.0, 1.5, "Task 1: Acquisition & DSP Task (High Priority)", bg="#EDE9FE", edge="#5B21B6", fontsize=9,
         subtext="Wakes on DMA Notification / vTaskDelayUntil(); Applies Moving Average / EMA Filter")

draw_arrow(ax2, 2.6, 8.0, 2.6, 7.0, color=TEAL, label="FromISR Notify", fontsize=7.5)
draw_arrow(ax2, 7.4, 8.0, 7.4, 7.0, color=BLUE, label="Mutex Read", fontsize=7.5)

draw_box(ax2, 2.2, 3.4, 5.6, 1.2, "FreeRTOS Queue: sensor_queue", bg=AMBER, edge=ORANGE, fontsize=8.5,
         subtext="Thread-safe copy of SensorSample struct (seq, t_ms, values, flags)")

draw_arrow(ax2, 5.0, 5.5, 5.0, 4.6, color=ORANGE, label="xQueueSend()", fontsize=7.5)

draw_box(ax2, 0.5, 1.0, 9.0, 1.5, "Task 2: Telemetry & Logging Task (Normal Priority)", bg="#DCFCE7", edge="#166534", fontsize=9,
         subtext="Blocks on xQueueReceive(); Formats Packet; Transmits via Non-Blocking UART DMA")

draw_arrow(ax2, 5.0, 3.4, 5.0, 2.5, color="#166534", label="xQueueReceive()", fontsize=7.5)

plt.tight_layout()
plt.savefig(os.path.join(DIAGRAMS_DIR, "fig6_driver_and_rtos.png"), dpi=220)
plt.close()
