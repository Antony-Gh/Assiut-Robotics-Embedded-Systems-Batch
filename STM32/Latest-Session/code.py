import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

import docx
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml import OxmlElement, parse_xml
from docx.oxml.ns import qn, nsdecls

# ------------------------------------------------------------------------------
# DIRECTORIES & PALETTE SETUP
# ------------------------------------------------------------------------------
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
         subtext="GPIO Ports A..H,\nRCC, CRC, ADC (fast)")

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
draw_box(ax, 0.5, 8.2, 9.0, 1.1, "3.3V Supply Rail + External Pull-Up Resistors (Rp ~ 2.2k-4.7k)", bg="#FFF1F2", edge="#BE123C", fontsize=8)
draw_box(ax, 0.5, 5.4, 2.8, 2.0, "STM32 Master", bg=ICE, edge=BLUE, fontsize=8.5, subtext="SCL / SDA\n(Open-Drain)")
draw_box(ax, 3.7, 5.4, 2.8, 2.0, "MPU6050 IMU", bg=MINT, edge=TEAL, fontsize=8.5, subtext="7-bit: 0x68\nHAL: 0x68<<1")
draw_box(ax, 6.9, 5.4, 2.6, 2.0, "Temp Sensor", bg=MINT, edge=TEAL, fontsize=8.5, subtext="7-bit: 0x48\nHAL: 0x48<<1")
draw_box(ax, 0.5, 1.2, 9.0, 3.6, "I²C Register Read Transaction", bg="#F8FAFC", edge=SLATE, fontsize=8.5,
         subtext="START | Addr7+W | ACK | RegAddr | ACK |\nRESTART | Addr7+R | ACK | Data[0..k] | NACK | STOP\nRise Time: tr ≈ 0.8 * Rp * Cbus (Max 300ns @ 400kHz)")

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
    ("Application Layer (app.c)", "Calls MPU6050_ReadImu(&imu, &data); Evaluates pitch/roll/alarms\nZero knowledge of register hex addresses or I2C shift rules", 7.8, "#DCFCE7", "#166534"),
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

print("All 6 high-resolution technical diagrams generated and saved cleanly.")

# ------------------------------------------------------------------------------
# DOCUMENT INITIALIZATION & STYLING HELPERS
# ------------------------------------------------------------------------------
doc = Document()

for section in doc.sections:
    section.top_margin = Inches(0.8)
    section.bottom_margin = Inches(0.8)
    section.left_margin = Inches(0.8)
    section.right_margin = Inches(0.8)
    section.different_first_page_header_footer = True

C_NAVY = RGBColor(15, 37, 55)       # Primary Heading
C_BLUE = RGBColor(30, 78, 121)      # Secondary Heading
C_TEAL = RGBColor(0, 128, 128)      # Tertiary / Accent
C_SLATE = RGBColor(51, 65, 85)      # Slate Dark
C_BODY = RGBColor(30, 41, 59)       # Body Text
C_MUTED = RGBColor(100, 116, 139)   # Captions

style_normal = doc.styles['Normal']
style_normal.font.name = 'Segoe UI'
style_normal.font.size = Pt(10)
style_normal.font.color.rgb = C_BODY
style_normal.paragraph_format.space_after = Pt(5)
style_normal.paragraph_format.line_spacing = 1.18

h1_style = doc.styles['Heading 1']
h1_style.font.name = 'Segoe UI'
h1_style.font.size = Pt(17)
h1_style.font.bold = True
h1_style.font.color.rgb = C_NAVY
h1_style.paragraph_format.space_before = Pt(16)
h1_style.paragraph_format.space_after = Pt(6)
h1_style.paragraph_format.keep_with_next = True

h2_style = doc.styles['Heading 2']
h2_style.font.name = 'Segoe UI'
h2_style.font.size = Pt(13)
h2_style.font.bold = True
h2_style.font.color.rgb = C_BLUE
h2_style.paragraph_format.space_before = Pt(12)
h2_style.paragraph_format.space_after = Pt(4)
h2_style.paragraph_format.keep_with_next = True

h3_style = doc.styles['Heading 3']
h3_style.font.name = 'Segoe UI'
h3_style.font.size = Pt(11)
h3_style.font.bold = True
h3_style.font.color.rgb = C_TEAL
h3_style.paragraph_format.space_before = Pt(9)
h3_style.paragraph_format.space_after = Pt(3)
h3_style.paragraph_format.keep_with_next = True

header = doc.sections[0].header
hp = header.paragraphs[0]
hp.alignment = WD_ALIGN_PARAGRAPH.RIGHT
hrun = hp.add_run("STM32 Embedded Systems — Complete Technical Reference & Training Manual")
hrun.font.name = 'Segoe UI'
hrun.font.size = Pt(8.5)
hrun.font.color.rgb = C_MUTED

footer = doc.sections[0].footer
fp = footer.paragraphs[0]
fp.alignment = WD_ALIGN_PARAGRAPH.CENTER
frun1 = fp.add_run("STM32CubeIDE 2.2.0 • STM32CubeMX 6.18.1 • HAL/LL/Registers • DMA • Timers • Protocols • Drivers • FreeRTOS    |    Page ")
frun1.font.name = 'Segoe UI'
frun1.font.size = Pt(8.5)
frun1.font.color.rgb = C_MUTED

def add_xml_field(p, field_str):
    run = p.add_run()
    run.font.name = 'Segoe UI'
    run.font.size = Pt(8.5)
    run.font.color.rgb = C_MUTED
    fldChar1 = parse_xml(r'<w:fldChar %s w:fldCharType="begin"/>' % nsdecls('w'))
    instrText = parse_xml(r'<w:instrText %s xml:space="preserve"> %s </w:instrText>' % (nsdecls('w'), field_str))
    fldChar2 = parse_xml(r'<w:fldChar %s w:fldCharType="separate"/>' % nsdecls('w'))
    fldChar3 = parse_xml(r'<w:fldChar %s w:fldCharType="end"/>' % nsdecls('w'))
    run._r.append(fldChar1)
    run._r.append(instrText)
    run._r.append(fldChar2)
    run._r.append(fldChar3)

add_xml_field(fp, "PAGE")
frun2 = fp.add_run(" of ")
frun2.font.name = 'Segoe UI'
frun2.font.size = Pt(8.5)
frun2.font.color.rgb = C_MUTED
add_xml_field(fp, "NUMPAGES")

def set_cell_bg(cell, hex_color):
    tcPr = cell._tc.get_or_add_tcPr()
    shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{hex_color}"/>')
    tcPr.append(shd)

def set_cell_margins(cell, top=100, bottom=100, left=140, right=140):
    tcPr = cell._tc.get_or_add_tcPr()
    tcMar = OxmlElement('w:tcMar')
    for m, val in [('top', top), ('bottom', bottom), ('left', left), ('right', right)]:
        node = OxmlElement(f'w:{m}')
        node.set(qn('w:w'), str(val))
        node.set(qn('w:type'), 'dxa')
        tcMar.append(node)
    tcPr.append(tcMar)

def prevent_row_split(row):
    trPr = row._tr.get_or_add_trPr()
    trPr.append(OxmlElement('w:cantSplit'))

def repeat_header_row(row):
    trPr = row._tr.get_or_add_trPr()
    trPr.append(OxmlElement('w:tblHeader'))

def add_h1(text, page_break=False):
    if page_break:
        doc.add_page_break()
    p = doc.add_heading(text, level=1)
    pPr = p._p.get_or_add_pPr()
    pBdr = parse_xml(f'<w:pBdr {nsdecls("w")}><w:bottom w:val="single" w:sz="12" w:space="4" w:color="008080"/></w:pBdr>')
    pPr.append(pBdr)
    return p

def add_h2(text):
    return doc.add_heading(text, level=2)

def add_h3(text):
    return doc.add_heading(text, level=3)

def add_p(text, bold_prefix=None, italic=False, space_after=5):
    p = doc.add_paragraph()
    p.paragraph_format.space_after = Pt(space_after)
    if bold_prefix:
        rb = p.add_run(bold_prefix)
        rb.bold = True
        rb.font.color.rgb = C_NAVY
    r = p.add_run(text)
    r.italic = italic
    return p

def add_bullet(text, bold_prefix=None, level=0):
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.left_indent = Inches(0.28 * (level + 1))
    p.paragraph_format.space_after = Pt(3)
    if bold_prefix:
        rb = p.add_run(bold_prefix)
        rb.bold = True
        rb.font.color.rgb = C_NAVY
    p.add_run(text)
    return p

def add_callout(title, body_text, kind="NOTE"):
    palettes = {
        "NOTE":    ("F0F9FF", "0284C7", RGBColor(3, 105, 161)),
        "TIP":     ("F0FDFA", "0D9488", RGBColor(15, 118, 110)),
        "WARNING": ("FFFBEB", "D97706", RGBColor(180, 83, 9)),
        "DANGER":  ("FEF2F2", "DC2626", RGBColor(185, 28, 28)),
        "CONCEPT": ("F1F5F9", "0F2537", RGBColor(15, 37, 55)),
    }
    bg_hex, border_hex, title_rgb = palettes.get(kind, palettes["NOTE"])
    tbl = doc.add_table(rows=1, cols=1)
    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    tbl.autofit = False
    tbl.columns[0].width = Inches(6.9)
    cell = tbl.cell(0, 0)
    cell.width = Inches(6.9)
    set_cell_bg(cell, bg_hex)
    set_cell_margins(cell, top=110, bottom=110, left=180, right=150)
    tcPr = cell._tc.get_or_add_tcPr()
    tcBorders = parse_xml(f'''
        <w:tcBorders {nsdecls("w")}>
            <w:top w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
            <w:left w:val="single" w:sz="24" w:space="0" w:color="{border_hex}"/>
            <w:bottom w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
            <w:right w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        </w:tcBorders>
    ''')
    tcPr.append(tcBorders)
    prevent_row_split(tbl.rows[0])
    cp = cell.paragraphs[0]
    cp.paragraph_format.space_after = Pt(2)
    rt = cp.add_run(f"[{kind}] {title}: ")
    rt.bold = True
    rt.font.name = 'Segoe UI'
    rt.font.size = Pt(9.5)
    rt.font.color.rgb = title_rgb
    rb = cp.add_run(body_text)
    rb.font.name = 'Segoe UI'
    rb.font.size = Pt(9.5)
    rb.font.color.rgb = C_BODY
    sp = doc.add_paragraph()
    sp.paragraph_format.space_after = Pt(3)

def add_code_block(title, code_text):
    p_cap = doc.add_paragraph()
    p_cap.paragraph_format.space_before = Pt(6)
    p_cap.paragraph_format.space_after = Pt(2)
    p_cap.paragraph_format.keep_with_next = True
    rc = p_cap.add_run(f"Code Listing — {title}")
    rc.bold = True
    rc.font.name = 'Segoe UI'
    rc.font.size = Pt(9)
    rc.font.color.rgb = C_BLUE

    tbl = doc.add_table(rows=1, cols=1)
    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    tbl.autofit = False
    tbl.columns[0].width = Inches(6.9)
    cell = tbl.cell(0, 0)
    cell.width = Inches(6.9)
    set_cell_bg(cell, "F8FAFC")
    set_cell_margins(cell, top=100, bottom=100, left=140, right=140)
    tcPr = cell._tc.get_or_add_tcPr()
    tcBorders = parse_xml(f'''
        <w:tcBorders {nsdecls("w")}>
            <w:top w:val="single" w:sz="6" w:space="0" w:color="94A3B8"/>
            <w:left w:val="single" w:sz="16" w:space="0" w:color="1E4E79"/>
            <w:bottom w:val="single" w:sz="6" w:space="0" w:color="94A3B8"/>
            <w:right w:val="single" w:sz="6" w:space="0" w:color="94A3B8"/>
        </w:tcBorders>
    ''')
    tcPr.append(tcBorders)
    cp = cell.paragraphs[0]
    cp.paragraph_format.space_after = Pt(0)
    cp.paragraph_format.line_spacing = 1.08
    r = cp.add_run(code_text.strip())
    r.font.name = 'Consolas'
    r.font.size = Pt(8.5)
    r.font.color.rgb = RGBColor(15, 23, 42)
    sp = doc.add_paragraph()
    sp.paragraph_format.space_after = Pt(4)

def add_formula_box(title, formula_lines, explanation_list):
    tbl = doc.add_table(rows=1, cols=1)
    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    tbl.autofit = False
    tbl.columns[0].width = Inches(6.9)
    cell = tbl.cell(0, 0)
    cell.width = Inches(6.9)
    set_cell_bg(cell, "F0F6FA")
    set_cell_margins(cell, top=100, bottom=100, left=160, right=160)
    tcPr = cell._tc.get_or_add_tcPr()
    tcBorders = parse_xml(f'''
        <w:tcBorders {nsdecls("w")}>
            <w:top w:val="single" w:sz="6" w:space="0" w:color="008080"/>
            <w:left w:val="single" w:sz="18" w:space="0" w:color="008080"/>
            <w:bottom w:val="single" w:sz="6" w:space="0" w:color="008080"/>
            <w:right w:val="single" w:sz="6" w:space="0" w:color="008080"/>
        </w:tcBorders>
    ''')
    tcPr.append(tcBorders)
    prevent_row_split(tbl.rows[0])
    p0 = cell.paragraphs[0]
    p0.paragraph_format.space_after = Pt(3)
    rt = p0.add_run(f"Mathematical Formulation — {title}")
    rt.bold = True
    rt.font.size = Pt(9.5)
    rt.font.color.rgb = C_NAVY
    for fl in formula_lines:
        pf = cell.add_paragraph()
        pf.alignment = WD_ALIGN_PARAGRAPH.CENTER
        pf.paragraph_format.space_after = Pt(3)
        rf = pf.add_run(fl)
        rf.bold = True
        rf.font.name = 'Cambria Math'
        rf.font.size = Pt(10.5)
        rf.font.color.rgb = C_BLUE
    for exp in explanation_list:
        pe = cell.add_paragraph()
        pe.paragraph_format.space_after = Pt(2)
        re = pe.add_run("• " + exp)
        re.font.size = Pt(8.8)
        re.font.color.rgb = C_SLATE
    sp = doc.add_paragraph()
    sp.paragraph_format.space_after = Pt(4)

def add_styled_table(caption, headers, rows, col_widths=None):
    p_cap = doc.add_paragraph()
    p_cap.paragraph_format.space_before = Pt(6)
    p_cap.paragraph_format.space_after = Pt(2)
    p_cap.paragraph_format.keep_with_next = True
    rc = p_cap.add_run(caption)
    rc.bold = True
    rc.font.size = Pt(9.5)
    rc.font.color.rgb = C_NAVY

    ncols = len(headers)
    tbl = doc.add_table(rows=len(rows) + 1, cols=ncols)
    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    tbl.autofit = False

    if not col_widths:
        w = 6.9 / ncols
        col_widths = [w] * ncols

    tblPr = tbl._tbl.tblPr
    tblBorders = parse_xml(f'''
        <w:tblBorders {nsdecls("w")}>
            <w:top w:val="single" w:sz="8" w:space="0" w:color="0F2537"/>
            <w:left w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
            <w:bottom w:val="single" w:sz="8" w:space="0" w:color="0F2537"/>
            <w:right w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
            <w:insideH w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
            <w:insideV w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        </w:tblBorders>
    ''')
    tblPr.append(tblBorders)

    hdr_row = tbl.rows[0]
    repeat_header_row(hdr_row)
    prevent_row_split(hdr_row)
    for j, htext in enumerate(headers):
        cell = hdr_row.cells[j]
        cell.width = Inches(col_widths[j])
        set_cell_bg(cell, "0F2537")
        set_cell_margins(cell, top=85, bottom=85, left=100, right=100)
        p = cell.paragraphs[0]
        p.paragraph_format.space_after = Pt(0)
        r = p.add_run(str(htext))
        r.bold = True
        r.font.name = 'Segoe UI'
        r.font.size = Pt(9)
        r.font.color.rgb = RGBColor(255, 255, 255)

    for i, rdata in enumerate(rows):
        row = tbl.rows[i + 1]
        prevent_row_split(row)
        bg = "FFFFFF" if i % 2 == 0 else "F8FAFC"
        for j, val in enumerate(rdata):
            cell = row.cells[j]
            cell.width = Inches(col_widths[j])
            set_cell_bg(cell, bg)
            set_cell_margins(cell, top=70, bottom=70, left=100, right=100)
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(0)
            p.paragraph_format.line_spacing = 1.12
            r = p.add_run(str(val))
            r.font.name = 'Segoe UI'
            r.font.size = Pt(8.8)
            if j == 0:
                r.bold = True
                r.font.color.rgb = C_NAVY
            else:
                r.font.color.rgb = C_BODY

    sp = doc.add_paragraph()
    sp.paragraph_format.space_after = Pt(4)

def add_figure(img_rel_path, caption_text, width_in=6.7):
    p_img = doc.add_paragraph()
    p_img.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_img.paragraph_format.space_before = Pt(6)
    p_img.paragraph_format.space_after = Pt(2)
    p_img.paragraph_format.keep_with_next = True
    run_img = p_img.add_run()
    img_abs_path = os.path.join(DIAGRAMS_DIR, os.path.basename(img_rel_path))
    run_img.add_picture(img_abs_path, width=Inches(width_in))

    p_cap = doc.add_paragraph()
    p_cap.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_cap.paragraph_format.space_after = Pt(8)
    rc = p_cap.add_run(caption_text)
    rc.italic = True
    rc.bold = True
    rc.font.size = Pt(8.8)
    rc.font.color.rgb = C_MUTED

print("DOCX helper architecture initialized.")

# ==============================================================================
# COVER PAGE
# ==============================================================================
cov_tbl = doc.add_table(rows=1, cols=1)
cov_tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
cov_tbl.autofit = False
cov_tbl.columns[0].width = Inches(6.9)
cov_cell = cov_tbl.cell(0, 0)
cov_cell.width = Inches(6.9)
set_cell_bg(cov_cell, "0F2537")
set_cell_margins(cov_cell, top=360, bottom=360, left=280, right=280)

cp1 = cov_cell.paragraphs[0]
cp1.paragraph_format.space_after = Pt(8)
r_badge = cp1.add_run("DEFINITIVE TECHNICAL TEXTBOOK & PRACTICAL TRAINING MANUAL")
r_badge.font.name = 'Segoe UI'
r_badge.font.size = Pt(10)
r_badge.font.bold = True
r_badge.font.color.rgb = RGBColor(56, 189, 248)

cp2 = cov_cell.add_paragraph()
cp2.paragraph_format.space_after = Pt(10)
r_title = cp2.add_run("STM32 EMBEDDED SYSTEMS\nCOMPLETE PROFESSIONAL REFERENCE")
r_title.font.name = 'Segoe UI'
r_title.font.size = Pt(24)
r_title.font.bold = True
r_title.font.color.rgb = RGBColor(255, 255, 255)

cp3 = cov_cell.add_paragraph()
cp3.paragraph_format.space_after = Pt(14)
r_sub = cp3.add_run("From STM32CubeIDE 2.2.0 & STM32CubeMX 6.18.1 Configuration to Deterministic Real-Time Data Acquisition, Interrupts, DMA, Timers, Serial Protocols (UART/SPI/I²C/CAN/USB), Reusable Sensor Drivers, Digital Signal Processing, and FreeRTOS Architecture")
r_sub.font.name = 'Segoe UI'
r_sub.font.size = Pt(11.5)
r_sub.font.color.rgb = RGBColor(226, 232, 240)

cp4 = cov_cell.add_paragraph()
cp4.paragraph_format.space_after = Pt(0)
r_tags = cp4.add_run("CONFIGURE (Pins • Clocks • .ioc)   |   IMPLEMENT (HAL • LL • Registers • ISR • DMA)   |   ENGINEER (ADC • Drivers • DSP • FreeRTOS)")
r_tags.font.name = 'Segoe UI'
r_tags.font.size = Pt(9)
r_tags.font.bold = True
r_tags.font.color.rgb = RGBColor(45, 212, 191)

doc.add_paragraph()

pillar_tbl = doc.add_table(rows=1, cols=3)
pillar_tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
pillar_tbl.autofit = False
for idx, (p_title, p_desc, bg_hex, bdr_hex) in enumerate([
    ("1. Hardware & Configuration", "MCU Architecture • AHB/APB Bus Matrix • STM32CubeMX 6.18.1 • .ioc Workflow • Clock Tree & APB x2 Timer Rule • GPIO & 5V Tolerance", "F0F9FF", "0284C7"),
    ("2. Peripherals & Data Path", "HAL vs. LL vs. CMSIS Registers • NVIC Preemption • DMA Channels/Streams/DMAMUX • Timers & PWM • UART/SPI/I²C • SAR ADC & AFE", "F0FDFA", "0D9488"),
    ("3. Architecture & Real-Time", "Deterministic Timer+ADC+DMA Ping-Pong • Moving Average & EMA DSP • 4-Layer Reusable Sensor Drivers (MPU6050) • FreeRTOS Tasks/Queues/Mutexes", "F8FAFC", "0F2537")
]):
    c = pillar_tbl.cell(0, idx)
    c.width = Inches(2.3)
    set_cell_bg(c, bg_hex)
    set_cell_margins(c, top=120, bottom=120, left=120, right=120)
    tcPr = c._tc.get_or_add_tcPr()
    tcPr.append(parse_xml(f'<w:tcBorders {nsdecls("w")}><w:top w:val="single" w:sz="18" w:space="0" w:color="{bdr_hex}"/><w:left w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/><w:bottom w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/><w:right w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/></w:tcBorders>'))
    p = c.paragraphs[0]
    p.paragraph_format.space_after = Pt(4)
    rt = p.add_run(p_title + "\n")
    rt.bold = True
    rt.font.size = Pt(9.5)
    rt.font.color.rgb = C_NAVY
    rb = p.add_run(p_desc)
    rb.font.size = Pt(8.5)
    rb.font.color.rgb = C_SLATE

doc.add_paragraph()

add_styled_table(
    "Document Information, Toolchain Baseline, and Version Control",
    ["Attribute", "Specification / Baseline", "Engineering Notes & Scope"],
    [
        ["Document Title", "STM32 Complete Professional Reference", "Consolidated University-Level Textbook & Practical Field Manual"],
        ["Document ID & Version", "DOC-STM32-REF-2026 • Version 4.0 (Definitive Master Edition)", "Reconciles and integrates all 7 session & deep-dive source files"],
        ["Target Toolchain", "STM32CubeIDE 2.2.0 • STM32CubeMX 6.18.1 • GNU Arm (arm-none-eabi-gcc)", "Covers CMSIS, STM32 HAL, Low-Layer (LL), and Direct Register Access"],
        ["Hardware Scope", "Family-Aware across STM32F1, F4, G0, G4, L4, H7, and U5 Series", "Explicitly highlights F1 (STM32F103C8T6), F4, and Cortex-M7 (H7) nuances"],
        ["Target Audience", "Embedded Systems Engineers, Robotics Teams, University Students & Instructors", "Designed for both self-paced study and 90-min / 2.5-hr / 6-hr workshops"],
        ["Pedagogical Rule", "Problem -> Concept -> Engineering Choice -> CubeMX -> Code -> Verify -> Debug", "Prioritizes deterministic timing, data ownership, and hardware verification"]
    ],
    col_widths=[1.6, 2.5, 2.8]
)

# ==============================================================================
# FRONT MATTER: SOURCE CONSOLIDATION & TABLE OF CONTENTS
# ==============================================================================
add_h1("Document Consolidation & Source Coverage Matrix", page_break=True)
add_p("This reference manual represents a complete knowledge consolidation across seven foundational technical documents, instructor guides, and deep-dive engineering textbooks. Every unique concept, mathematical derivation, hardware caveat, code pattern, lab exercise, and debugging checklist has been preserved, cross-verified against official STMicroelectronics and ARM documentation, and unified into a single coherent architecture.")

add_styled_table(
    "Table 0.1 — Consolidated Source Files & Knowledge Integration Checklist",
    ["Source File", "Primary Focus", "Unique Technical Contributions Preserved & Integrated"],
    [
        ["1. Overview.md", "Session Strategy & Engineering Judgment", "Member expectations analysis; Arabic technical judgment quote; 2.5-hr & 90-min schedules; CAN/USB/Ethernet protocol comparisons; Scenarios A–E; Digital Signal Processing (MA/EMA filters, Nyquist); 10-step live demo plan."],
        ["2. STM32_Session_Instructor_Guide.docx", "6-Hour Evening Workshop Instructor Guide", "6-hour schedule (18:00–00:00 with prayer break); 6-question Instructor Rule; 28-term CubeMX glossary; HAL vs. BSRR PC13 example; STM32F103C8T6 72 MHz timer math & 16-bit PSC overflow trap (PSC=71999 > 65535)."],
        ["3. STM32_Session_Complete_Guide.md", "Expanded Instructor & Student Reference", "IWDG vs. WWDG watchdog best practices (no unconditional ISR refresh); RTC vs. TIM vs. SysTick; per-peripheral parameter checklists; multi-channel scan vs. trigger rate math; subsystem failure checklists."],
        ["4. STM32_Deep_Dive_Complete_Guide.docx", "22-Chapter Field Guide (DOCX Edition)", "6-layer system chain; compile/link/flash/run/debug proof matrix; UART interrupt line parser with overflow counter; TempSensor driver struct/enum; FreeRTOS producer/consumer queue pattern; 7 structured labs."],
        ["5. STM32_Deep_Dive_Complete_Guide.md", "22-Chapter Field Guide (Markdown Edition)", "Complete C code listings for timer-triggered ADC+DMA (200 samples @ 10 kS/s = 10 ms half-buffer deadline); SPI full-duplex dummy byte transaction; I²C 7-bit << 1 shift & error inspection."],
        ["6. STM32_Deep_Dive_Complete_Guide_Compact.md", "Compact Field Checklist Companion", "Verified 100% parity with Complete Guide; consolidated into quick-reference engineering checklists, formula tables, and 20 oral/written assessment questions."],
        ["7. STM32_Deep_Dive_In_Depth.md", "Silicon-Level & Architectural Textbook", "10-layer physical-to-actuation chain; Data Ownership rules; NTC Steinhart-Hart chain; F1/F4/G4/H7 DMA & DMAMUX differences; Cortex-M7 D-Cache coherency; I²C rise-time math & 9-clock recovery; HardFault register forensics (CFSR/BFAR/MMFAR)."]
    ],
    col_widths=[1.9, 1.6, 3.4]
)

add_h2("Master Table of Contents")
add_p("Note for Microsoft Word Users: Right-click the dynamic Table of Contents field below and select 'Update Field' -> 'Update entire table' at any time to refresh page numbers. A complete structural outline is also provided immediately below for instant navigation across all document viewers.", italic=True)

p_toc = doc.add_paragraph()
add_xml_field(p_toc, 'TOC \\o "1-3" \\h \\z \\u')

add_styled_table(
    "Master Structural Outline of This Reference",
    ["Part / Section", "Title", "Core Subtopics Covered"],
    [
        ["Section 0", "Foundations, Learning Objectives & System View", "Pedagogical Philosophy • 6-Hr / 2.5-Hr / 90-Min Schedules • 10-Layer Signal Chain • Data Ownership • NTC Telemetry Walkthrough"],
        ["Part 1", "STM32 Development Workflow, Architecture & HAL vs. Registers", "Cortex-M Bus Matrix • STM32CubeIDE 2.2.0 & CubeMX 6.18.1 • .ioc Workflow • GPIO & FT Pins • Clock Tree & APB x2 Rule • Codegen & MSP • HAL vs. LL vs. Registers"],
        ["Part 2", "DMA, Interrupts (NVIC), and Hardware Timers", "Polling vs. IRQ vs. DMA • NVIC Preemption & ISR Rules • DMA Architecture (Channels/Streams/DMAMUX, Ping-Pong, Cache) • Timers (PSC/ARR/CCR, PWM, Input Capture, TRGO) • F103 Worked Math • Timer+ADC+DMA Pipeline"],
        ["Part 3", "Communication Protocols (UART, SPI, I²C, CAN, USB)", "UART (8-N-1, BRR Math, Ring Buffers, Idle-DMA, Error Flags) • SPI (Modes 0–3, CPOL/CPHA, Dummy Bytes, GPIO CS) • I²C (Open-Drain Pull-Ups, 7-Bit Shift, Mem_Read, 9-Clock Recovery) • Protocol Selection & Scenarios A–E"],
        ["Part 4", "Sensors, ADC, Real-Time Acquisition & DSP", "Sensor Taxonomy • SAR ADC Fundamentals & Equations • Analog Front-End (Impedance, Crosstalk, Calibration, 5V Safety) • Digital Sensors (MPU6050 I²C & SPI) • Throughput & Buffering • DSP (Nyquist, Moving Average, EMA Filters)"],
        ["Part 5", "Sensor Drivers & Reusable Embedded Software", "4-Layer Architecture • Folder Structure • 10 Golden API Rules • Reusable TempSensor Driver • Production MPU6050 6-Axis IMU Driver • Function-Pointer Bus Abstraction • Unit Testing & Mocking"],
        ["Part 6", "Real-Time Operating Systems (RTOS) & FreeRTOS", "Superloop vs. Non-Blocking FSM vs. RTOS • Tasks, States & RMS Scheduling • Context Switching (PendSV/SVC) & HAL Timebase • Queues, Semaphores, Mutexes & Task Notifications • FromISR & Priority Rules • Stacks & Heap"],
        ["Part 7", "Capstone System, Debugging Playbook, Labs, Glossary & Q&A", "Integrated Multi-Sensor System • 10-Layer Debugging Playbook & HardFault Forensics • 7 Hands-On Labs + Capstone • Checklists & Master Formulas • 35-Term Glossary • 22 Knowledge Check Q&As • Official References"]
    ],
    col_widths=[1.0, 2.3, 3.6]
)

add_styled_table(
    "List of Embedded Technical Diagrams & Figures",
    ["Figure", "Title", "Section"],
    [
        ["Figure 1.1", "A. 10-Layer Embedded Signal & Ownership Chain  |  B. STM32 Internal Architecture & Multi-AHB Bus Matrix", "Section 0.4 / 1.1"],
        ["Figure 1.2", "A. STM32CubeMX (.ioc) to Firmware Lifecycle  |  B. Software Abstraction Stack (Application to Silicon)", "Section 1.3 / 1.7"],
        ["Figure 2.1", "Execution Paradigms: Polling vs. Interrupt-Driven vs. Circular DMA (Ping-Pong Buffer Ownership Timeline)", "Section 2.1 / 2.3"],
        ["Figure 2.2", "A. STM32 Hardware Timer Core Architecture  |  B. Deterministic Pipeline: Timer TRGO -> SAR ADC -> Circular DMA -> DSP Task", "Section 2.4 / 2.6"],
        ["Figure 3.1", "Serial Communication Topologies & Frames: A. UART (8-N-1)  |  B. SPI (Full-Duplex Modes 0–3)  |  C. I²C (Open-Drain Shared Bus)", "Section 3.1 – 3.3"],
        ["Figure 5.1", "A. 4-Layer Reusable Sensor Driver Architecture  |  B. FreeRTOS Real-Time Sensor Monitoring Architecture", "Section 5.1 / 6.7"]
    ],
    col_widths=[1.0, 4.7, 1.2]
)

# ==============================================================================
# SECTION 0: LEARNING OBJECTIVES, SESSION PLANS & THE SYSTEM VIEW
# ==============================================================================
add_h1("0. Learning Objectives, Pedagogical Strategy & The System View", page_break=True)

add_h2("0.1 Why Engineering Decisions Matter More Than Peripheral Lists")
add_p("A common pitfall in embedded systems training is treating a microcontroller as a disconnected vocabulary list of peripheral acronyms—memorizing definitions of DMA, ADC, TIM, UART, SPI, I²C, and FreeRTOS without understanding how to synthesize them into a deterministic system. When engineering students transition from basic GPIO blinking to real-world robotics, autonomous vehicles, and industrial telemetry, their primary challenge is technical judgment:")
add_callout("Core Student Insight", "« هو مش شرح ليهم من الصفر ... هو بس امتى افكر مثلا اني استخدم ال DMA وامتي اعتمد علي ال adc » — 'The goal is not explaining them from scratch... it is knowing when I should think about using DMA and when to rely on basic ADC reads.' Every peripheral exists to solve a specific hardware timing, bandwidth, or CPU-load bottleneck.", "CONCEPT")

add_p("To build genuine engineering competence, every topic in this reference follows a strict seven-step instructional and engineering loop:")
add_bullet("What physical signal exists, what must the firmware produce, how often, and what happens on failure?", "1. Define the Hardware Problem: ")
add_bullet("Explain why the hardware block exists and what bottleneck it eliminates.", "2. Core Concept & Internal Mechanism: ")
add_bullet("Evaluate trade-offs (Polling vs. IRQ vs. DMA; UART vs. SPI vs. I²C; Superloop vs. FreeRTOS).", "3. Engineering Decision Criteria: ")
add_bullet("Consult the exact MCU Datasheet (pinout, electrical limits, Alternate Functions) and Reference Manual (clock tree, registers, DMA mapping) before clicking inside STM32CubeMX.", "4. Datasheet & CubeMX Configuration: ")
add_bullet("Write clean, modular C code that checks return statuses, respects buffer ownership, and avoids blocking critical paths.", "5. Firmware Implementation: ")
add_bullet("Verify behavior with an oscilloscope, logic analyzer, multimeter, or diagnostic GPIO toggle. Calculation without measurement is merely a hypothesis.", "6. Hardware Verification & Measurement: ")
add_bullet("Anticipate common configuration traps, electrical faults, race conditions, and bus lockups.", "7. Failure Modes & Debugging: ")

add_h2("0.2 Session Learning Outcomes & Delivery Schedules")
add_p("Whether used for independent study or an instructor-led training batch, this material is structured around three primary learning outcomes:")
add_bullet("Explain precisely when and why to use a hardware timer, SAR ADC, DMA controller, NVIC interrupt, or serial protocol—and how they complement rather than replace one another.", "Outcome 1 — Choose the Right Hardware Feature: ")
add_bullet("Configure peripherals in STM32CubeMX 6.18.1, navigate STM32CubeIDE 2.2.0 generated code, preserve user logic across regenerations, and architect hardware-agnostic reusable sensor drivers.", "Outcome 2 — Write Maintainable, Modular Embedded Software: ")
add_bullet("Design, calculate, and verify end-to-end deterministic pipelines combining timer-triggered sampling, ping-pong DMA buffers, digital filtering (DSP), and FreeRTOS task/queue synchronization.", "Outcome 3 — Architect Deterministic Real-Time Systems: ")

add_styled_table(
    "Table 0.2 — Comprehensive 6-Hour Evening Workshop Schedule (Instructor Delivery Plan)",
    ["Time Slot", "Session Block", "Duration", "Key Topics & Student Outcomes"],
    [
        ["18:00 – 18:30", "0. Arrival & Toolchain Check", "30 min", "Verify STM32CubeIDE 2.2.0, CubeMX 6.18.1, MCU packages, ST-LINK drivers, and target hardware."],
        ["18:30 – 19:10", "Part 1: Workflow & Architecture", "40 min", "CubeMX, .ioc, Clock Tree & APB x2 rule, GPIO electrical limits, Codegen, HAL vs. LL vs. Registers."],
        ["19:10 – 20:00", "Part 2: Interrupts, DMA & Timers", "50 min", "Polling vs. IRQ vs. DMA, NVIC priorities, Timer math (PSC/ARR/CCR), Timer TRGO + ADC + DMA pipeline."],
        ["20:00 – 20:30", "Prayer & Rest Break", "30 min", "Scheduled rest and prayer break (adjust to local prayer times); informal Q&A."],
        ["20:30 – 21:10", "Part 3: UART, SPI & I²C Protocols", "40 min", "Framing, baud rate math, SPI modes (CPOL/CPHA), I²C open-drain pull-ups & addressing, protocol selection."],
        ["21:10 – 21:45", "Part 4: Sensors, ADC & DSP", "35 min", "SAR ADC equations, source impedance, MPU6050 register reads, throughput math, MA/EMA digital filters."],
        ["21:45 – 22:20", "Part 5: Reusable Sensor Drivers", "35 min", "4-layer architecture, dependency injection, status enums, header/source separation, unit testing."],
        ["22:20 – 22:45", "Part 6: FreeRTOS Fundamentals", "25 min", "Superloop vs. RTOS, Tasks, Preemptive Scheduler, Queues, Semaphores, Mutexes, FromISR priority rules."],
        ["22:45 – 23:15", "Part 7: Integrated Capstone", "30 min", "End-to-end multi-sensor monitoring system synthesis and architectural design exercise."],
        ["23:15 – 00:00", "Debugging, Labs & Buffer", "45 min", "Live hardware troubleshooting, logic analyzer inspection, HardFault forensics, and open Q&A."]
    ],
    col_widths=[1.1, 1.7, 0.7, 3.4]
)

add_styled_table(
    "Table 0.3 — Alternative Condensed Schedules (2.5-Hour Standard Session & 90-Minute Intensive)",
    ["Section / Module", "2.5-Hour (150 Min) Allocation", "90-Minute Intensive Strategy"],
    [
        ["1. CubeIDE, CubeMX & .ioc Workflow", "15 minutes (Workflow & USER CODE preservation)", "10 minutes (Essential .ioc, clock tree & codegen rules)"],
        ["2. Timers, Interrupts & DMA", "30 minutes (Selection criteria & Timer+ADC+DMA)", "25 minutes (Core focus: Polling vs. IRQ vs. DMA & Timer math)"],
        ["3. Communication Protocols", "20 minutes (UART/SPI/I²C/CAN trade-offs & scenarios)", "15 minutes (Protocol decision matrix & I²C/SPI pitfalls)"],
        ["4. Sensors & Real-Time Acquisition", "25 minutes (ADC math, sampling, buffering & interfaces)", "15 minutes (ADC scaling, source impedance & DMA ping-pong)"],
        ["5. Reusable Sensor Drivers", "25 minutes (4-layer architecture & C driver pattern)", "15 minutes (Header/source API design & dependency injection)"],
        ["6. DSP & FreeRTOS Fundamentals", "25 minutes (10 min MA/EMA filtering + 15 min FreeRTOS)", "10 minutes (Executive overview of DSP filters & RTOS tasks/queues)"],
        ["7. Integrated System & Q&A", "10 minutes (System architecture synthesis)", "Included in wrap-up (Provide textbook for self-paced lab completion)"]
    ],
    col_widths=[2.1, 2.4, 2.4]
)

add_h2("0.3 The System as a 10-Layer Chain & Data Ownership")
add_p("An STM32 microcontroller is a complete System-on-Chip (SoC). Firmware does not 'talk to a sensor' in a single magic step; it configures analog and digital silicon blocks so a physical quantity in the environment becomes a conditioned electrical signal, a digital code in a peripheral register, an aligned word in SRAM, a validated physical measurement in SI units, and finally an actuation command or telemetry packet. If you cannot sketch this chain on paper, you cannot systematically debug it.")

add_figure("diagrams/fig1_system_architecture.png", "Figure 1.1 — (A) The 10-Layer Physical-to-Application Signal & Ownership Chain; (B) STM32 Internal Multi-AHB Bus Matrix & Clock Distribution.")

add_styled_table(
    "Table 0.4 — The 10-Layer Embedded System View & Failure Modes",
    ["Layer", "Core Engineering Question", "Failure Mode If Ignored", "Concrete STM32 Example"],
    [
        ["1. Physical World", "What physical quantity exists, with what bandwidth and range?", "Sensor saturates or aliases high-frequency vibration.", "NTC thermistor measuring 0–100 °C or 1 kHz motor current ripple."],
        ["2. Electrical Network", "What impedance, voltage range, filtering, and grounding exist?", "ADC sample capacitor fails to charge; 5V destroys 3.3V pin.", "100 kOhm divider without buffer causes channel crosstalk; missing I²C pull-ups."],
        ["3. Pin & Pad Multiplexer", "Which package pin, GPIO mode, and Alternate Function (AF) is active?", "Schmitt trigger loads analog signal; peripheral disconnected.", "Leaving ADC pin in Digital Input mode instead of GPIO_MODE_ANALOG."],
        ["4. Peripheral Engine", "Which silicon block converts or shifts the signal, and at what clock?", "Wrong baud rate, wrong SPI phase, or ADC clock out of spec.", "ADC1 Channel 5 configured in software, but sensor wired to Channel 4."],
        ["5. Data Movement", "How do bytes reach RAM (Polling, ISR copy, or DMA)?", "CPU misses deadlines spinning on flags, or DMA writes wrong width.", "32-bit DMA word transfer into a 16-bit uint16_t array corrupts adjacent RAM."],
        ["6. Event Notification", "When is data valid, and how is software notified?", "Callback never fires because NVIC IRQ is disabled.", "Using DMA Half-Transfer (HT) and Transfer-Complete (TC) interrupts."],
        ["7. Buffer Ownership", "Which context owns the memory region, and until what deadline?", "Torn reads, race conditions, or circular DMA overwriting live data.", "Main loop reads adc_buf[] while circular DMA wraps around and overwrites it."],
        ["8. Driver & DSP", "How are raw codes validated, scaled to SI units, and filtered?", "Publishing stale data after bus NACK; treating raw 2048 as 20.48 °C.", "Checking WHO_AM_I, converting two's complement, applying EMA filter."],
        ["9. Application Policy", "What state transitions, thresholds, and fault recoveries apply?", "System hangs forever when a sensor wire disconnects.", "Hysteresis alarm state machine with bounded retry and safe-state fallback."],
        ["10. Output / Telemetry", "How is the result delivered, and can the output link keep up?", "Blocking UART print inside a 10 kHz ISR destroys sampling determinism.", "Streaming framed CSV packets (seq,t_ms,temp_c,flags) via non-blocking DMA."]
    ],
    col_widths=[1.2, 1.8, 1.9, 2.0]
)

add_callout("Data Ownership Is a First-Class Design Item", "Ownership defines which execution context (Main Loop, ISR, DMA Controller, or RTOS Task) is permitted to read or write a buffer at any given microsecond. A volatile ready flag is NOT a lock: it neither freezes a circular DMA controller nor makes multi-byte struct reads atomic. Always establish explicit ownership boundaries using ping-pong buffers, ring buffers with atomic single-writer/single-reader indices, or RTOS queues/mutexes.", "WARNING")

# ==============================================================================
# PART 1: STM32 DEVELOPMENT WORKFLOW, ARCHITECTURE & HAL VS. REGISTERS
# ==============================================================================
add_h1("1. Part 1 — STM32 Development Workflow, Architecture & Abstraction Layers", page_break=True)

add_h2("1.1 STM32 Internal Architecture & Multi-AHB Bus Matrix")
add_p("Modern STM32 microcontrollers are built around the 32-bit ARM Cortex-M processor family (such as Cortex-M0/M0+ in STM32C0/G0/L0, Cortex-M3 in STM32F1/F2, Cortex-M4F with hardware Floating-Point Unit and DSP instructions in STM32F3/F4/G4/L4, Cortex-M7 with L1 Instruction/Data Caches and dual-issue pipeline in STM32F7/H7, and Cortex-M33 with TrustZone in STM32U5/H5).")
add_p("Rather than routing all memory and peripheral traffic through a single bottlenecked bus, STM32 devices employ a Multi-Layer AHB (Advanced High-performance Bus) Interconnect Matrix (see Figure 1.1B):")
add_bullet("Connects the ARM Cortex-M core (via I-Code, D-Code, and System buses) and the DMA Controllers (Bus Masters) to Slave ports (Flash memory, SRAM banks, and AHB/APB peripherals). Because it is a crossbar matrix, the DMA controller can transfer ADC samples from APB2 into SRAM1 simultaneously while the CPU fetches instructions from Flash without bus contention.", "Multi-Layer AHB Bus Matrix: ")
add_bullet("Directly hosts high-speed functional blocks including GPIO Ports (GPIOA..GPIOH on most modern families), Reset and Clock Control (RCC), Cyclic Redundancy Check (CRC), and high-speed ADCs.", "AHB (Advanced High-performance Bus): ")
add_bullet("Bridges lower-speed peripherals (general-purpose timers TIM2–TIM7, USART2/3/4/5, I2C1/2/3, SPI2/3, CAN/FDCAN, DAC, PWR, IWDG, WWDG). On STM32F103, APB1 is limited to 36 MHz max; on STM32F401/F405, APB1 runs up to 42 MHz or 45 MHz.", "APB1 (Advanced Peripheral Bus 1 — Low Speed): ")
add_bullet("Bridges high-speed peripherals (advanced timers TIM1/TIM8, high-speed USART1/USART6, SPI1, ADC1/2/3, EXTI, SYSCFG). On STM32F103, APB2 runs up to full SYSCLK (72 MHz); on STM32F4, up to 84 MHz or 90 MHz.", "APB2 (Advanced Peripheral Bus 2 — High Speed): ")

add_h2("1.2 Toolchain Ecosystem: STM32CubeIDE 2.2.0 & STM32CubeMX 6.18.1")
add_p("Professional STM32 development relies on a coordinated toolchain where each component has a strict boundary of responsibility:")
add_bullet("A graphical hardware configuration and initialization code generator (available standalone or integrated inside CubeIDE). It manages MCU/board selection, pinout mapping, conflict resolution, clock-tree synthesis, peripheral parameterization, DMA/NVIC routing, and middleware configuration (FreeRTOS, USB, FatFS, LwIP). It stores all configuration state inside the human-readable project `.ioc` file.", "STM32CubeMX 6.18.1: ")
add_bullet("An Eclipse/CDT-based integrated development environment combining the CubeMX `.ioc` configurator, C/C++ code editor, build manager, and SWD/JTAG debugger interface (supporting ST-LINK v2/v3 and SEGGER J-Link) with Special Function Register (SFR) views, Live Expressions, Serial Wire Viewer (SWV/ITM) trace, and Fault Analyzer.", "STM32CubeIDE 2.2.0: ")
add_bullet("The industry-standard cross-compiler (`arm-none-eabi-gcc`), assembler (`as`), linker (`ld`), and binary utilities (`objcopy`, `size`) that translate C/Assembly sources into an executable `.elf` / `.bin` / `.hex` image mapped to the target silicon's Flash and SRAM.", "GNU Tools for STM32 (GCC Toolchain): ")

add_styled_table(
    "Table 1.1 — STM32 Toolchain Components: Responsibilities vs. Common Engineering Misconceptions",
    ["Tool / Artifact", "Actual Engineering Responsibility", "Common Misconception (What It Does NOT Do)"],
    [
        ["STM32CubeMX 6.18.1", "Pinout, Alternate Functions, Clock Tree, peripheral init structs, middleware setup, codegen.", "Does NOT write application state machines, frame serial packets, or handle fault recovery."],
        ["STM32CubeIDE 2.2.0", "Source editing, GCC compilation, linking, SWD flashing, breakpoint/register debugging.", "A warning-free compilation does NOT prove wiring, clock tree, or runtime timing correctness."],
        ["Project .ioc File", "Text-based blueprint storing target MCU part, pin mappings, clock dividers, and settings.", "It is NOT compiled firmware and is never flashed to the chip; must be committed to Git with sources."],
        ["CMSIS-Core / Device", "ARM standard register definitions (SCB, NVIC, SysTick) and device header (stm32f4xx.h).", "CMSIS is the register foundation, NOT the high-level ST HAL driver library."],
        ["STM32 HAL / LL Drivers", "ST's C driver abstractions (`HAL_UART_Transmit`, `LL_TIM_EnableCounter`) for a specific family.", "HAL APIs and peripheral capabilities are NOT identical across F1, F4, G4, and H7 families."],
        ["Startup File (startup_*.s)", "Assembly reset handler: initializes SP, copies .data from Flash to RAM, zeroes .bss, calls main().", "If the vector table or clock init crashes inside SystemInit(), execution never reaches main()."],
        ["Linker Script (*_FLASH.ld)", "Defines exact Flash/SRAM start addresses, sizes, and section placement (.text, .data, .bss, stack).", "Linking for a 512 KB variant while flashing a 128 KB chip causes immediate runtime HardFaults."],
        ["SWD Debug Probe", "Halts CPU, programs Flash sectors, reads/writes memory and peripheral registers over SWDIO/SWCLK.", "Halting at a breakpoint freezes the CPU while real-world sensors and timers continue moving."]
    ],
    col_widths=[1.6, 2.6, 2.7]
)

add_h2("1.3 Creating an STM32 Project & Mastering the .ioc Workflow")
add_figure("diagrams/fig2_workflow_and_hal.png", "Figure 1.2 — (A) STM32CubeMX (.ioc) to Firmware Build & Verification Lifecycle; (B) Software Abstraction Stack from Application to Silicon.")

add_p("When creating and configuring a project in STM32CubeIDE 2.2.0 / STM32CubeMX 6.18.1, follow this disciplined sequence:")
add_bullet("Read the laser marking on the IC package (e.g., `STM32F103C8T6` vs `STM32F401RCT6`) and inspect the board schematic. Identify the exact package (LQFP48, LQFP64, UFQFPN32) because smaller packages do not bond out all die pads. Verify whether an external crystal (HSE) is populated and its exact frequency.", "Step 1 — Verify Exact Silicon & Schematic: ")
add_bullet("Launch `File -> New -> STM32 Project`. Select the exact Commercial Part Number in the MCU/MPU Selector.", "Step 2 — Target Selection: ")
add_bullet("Under `System Core -> SYS`, ALWAYS configure the Debug interface (`Serial Wire` for 2-pin SWDIO/SWCLK). Warning: Leaving Debug set to `No Debug` disables PA13/PA14 SWD pins after reset, locking out the ST-LINK debugger!", "Step 3 — Configure SYS Debug & Timebase First: ")
add_bullet("If using FreeRTOS, change the `SYS Timebase Source` from `SysTick` to a dedicated hardware timer (such as `TIM6` or `TIM7`) so FreeRTOS owns `SysTick` while `HAL_Delay()` and HAL timeouts use the hardware timer without conflict.", "Step 4 — Select HAL Timebase Source: ")
add_bullet("Under `System Core -> RCC`, enable High Speed Clock (HSE) as `Crystal/Ceramic Resonator` or `BYPASS Clock Source`.", "Step 5 — Configure RCC Oscillators: ")
add_bullet("Assign GPIO pins, resolve alternate-function conflicts, and label pins with meaningful User Labels (e.g., `STATUS_LED`, `IMU_CS`, `FAULT_IN`).", "Step 6 — Pinout & User Labels: ")
add_bullet("Enable one peripheral at a time. Configure parameters, NVIC interrupt enables/priorities, and DMA request mappings.", "Step 7 — Peripheral, NVIC & DMA Configuration: ")
add_bullet("In the `Project Manager` tab under `Code Generator`, enable 'Generate peripheral initialization as a pair of .c/.h files per peripheral' for cleaner modular code, and ensure 'Keep User Code when re-generating' is checked.", "Step 8 — Project Manager Settings: ")
add_bullet("Save (`Ctrl+S`) to generate code. Immediately inspect `git diff` to verify changes.", "Step 9 — Generate, Inspect & Commit: ")

add_h2("1.4 GPIO Architecture, Alternate Functions & Electrical Behavior")
add_styled_table(
    "Table 1.3 — STM32 GPIO Operating Modes, Silicon Path, and Practical Engineering Uses",
    ["GPIO Mode", "Internal Silicon Configuration", "Electrical Behavior", "Primary Engineering Applications"],
    [
        ["Input Floating", "Output driver OFF; Schmitt trigger ON; Pull resistors OFF.", "High-impedance (Hi-Z); voltage floats wildly if undriven.", "Externally driven digital lines with external pull-ups/downs."],
        ["Input Pull-Up / Pull-Down", "Output driver OFF; Schmitt trigger ON; ~40 kOhm pull resistor ON.", "Weakly biases pin to VDD (Pull-Up) or GND (Pull-Down).", "Mechanical push-buttons, fault lines, unused digital inputs."],
        ["Output Push-Pull (PP)", "PMOS (top) and NMOS (bottom) transistors active.", "Actively drives both Logic 1 (VDD) and Logic 0 (GND).", "Driving indicator LEDs, SPI Chip-Select (CS), enable pins."],
        ["Output Open-Drain (OD)", "PMOS disabled; only NMOS transistor active.", "Actively pulls low (0V) or releases to Hi-Z; requires pull-up for 1.", "Bit-banged buses, multi-device wired-AND fault lines, level shifting."],
        ["Alternate Function PP", "Output Push-Pull controlled by internal peripheral MUX.", "Peripheral hardware drives high and low transitions.", "UART TX, SPI SCK/MOSI, Timer PWM outputs."],
        ["Alternate Function OD", "Output Open-Drain controlled by internal peripheral MUX.", "Peripheral pulls low or releases line; pull-up establishes high.", "I²C SCL and SDA lines, 1-Wire interfaces."],
        ["Analog Mode", "Output driver OFF; Schmitt trigger OFF; Pulls OFF; Analog switch ON.", "Zero digital switching noise; isolates pin for analog measurement.", "ADC voltage inputs, DAC outputs, comparator inputs, low-power sleep."]
    ],
    col_widths=[1.4, 1.8, 1.8, 1.9]
)

add_callout("Critical Electrical & 5V-Tolerance (FT) Rules", "1. Never assume every STM32 pin is 5V-tolerant: Check the exact MCU Datasheet Pinout table for the 'FT' (Five-Volt Tolerant) designation. Even on FT pins, 5V tolerance ONLY applies in digital input mode when VDD is powered—NEVER apply >3.3V (VDD) to a pin configured in Analog (ADC) mode! Driving an analog pin above VDD injects current into the internal rail, corrupting all ADC channels or latching up the silicon.\n2. GPIO Output Speed is Slew Rate: Setting GPIO speed to VERY_HIGH increases edge slew rate (dV/dt), which increases ringing, overshoot, and EMI on long PCB traces or ribbon cables. Use the lowest speed setting that satisfies your signal frequency.\n3. Current Limits: Individual GPIO pins are typically rated for 8–20 mA max (with strict total package VDD/VSS current limits around 80–120 mA). Never drive DC motors, solenoids, or relay coils directly from a GPIO pin; always use a MOSFET/BJT driver stage with a flyback protection diode.", "DANGER")

add_h2("1.5 Clock Tree Configuration, PLL, and the APB Timer x2 Multiplier Rule")
add_callout("The Famous STM32 APB Timer Clock x2 Multiplier Trap", "On classic STM32 architectures (F1, F2, F3, F4, F7, L4), timers connected to APB1 or APB2 do NOT always receive PCLK1 or PCLK2 directly! The hardware rule in the RCC chapter states:\n• If the APB prescaler is 1 (div1), then f_TIM = PCLK.\n• If the APB prescaler is > 1 (e.g., /2, /4, /8, /16), a hardware multiplier DOUBLES the clock fed to the timers: f_TIM = 2 x PCLK!\nExample (STM32F103 at 72 MHz): SYSCLK = 72 MHz, HCLK = 72 MHz, APB1 Prescaler = /2 -> PCLK1 = 36 MHz. Because APB1 prescaler != 1, TIM2/TIM3/TIM4 receive 36 MHz x 2 = 72 MHz! Always inspect the 'To APB1/APB2 Timer Clocks' box in CubeMX Clock Configuration.", "WARNING")

add_h2("1.6 Code Generation, Project Structure & USER CODE Preservation")
add_code_block("Protecting Custom Code Across CubeMX Regenerations in main.c", """
/* USER CODE BEGIN Includes */
#include "app.h"
#include "mpu6050.h"
/* USER CODE END Includes */

int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals (Ensure MX_DMA_Init precedes DMA users!) */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();

  /* USER CODE BEGIN 2 */
  App_Init();
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN WHILE */
    App_Update();
    /* USER CODE END WHILE */
  }
}
""")

add_h2("1.7 HAL vs. Low-Layer (LL) vs. Direct Register-Level Programming")
add_styled_table(
    "Table 1.4 — Comprehensive Comparison: STM32 HAL vs. Low-Layer (LL) vs. Direct Register Programming",
    ["Dimension", "STM32 HAL (Hardware Abstraction Layer)", "STM32 LL (Low-Layer Library)", "Direct CMSIS Register Access"],
    [
        ["Abstraction Level", "High: Peripheral handle structs (`UART_HandleTypeDef`), state machines, callbacks, and timeouts.", "Medium-Low: Thin `static inline` register-level helper functions with ST macro names.", "Lowest: Direct manipulation of memory-mapped peripheral struct fields (`USART2->CR1`)."],
        ["Development Speed", "Fastest: CubeMX generates complete init; complex peripherals (USB, CAN, I²C, DMA) work quickly.", "Moderate: CubeMX can generate LL init, but developer must manage state machines manually.", "Slowest: Requires reading every bitfield in the 1,500+ page Reference Manual."],
        ["Portability", "High across STM32 families (with minor handle/feature adjustments).", "Moderate: Closely tied to the specific peripheral IP version on that family.", "Low: Register names (`SR` vs `ISR`, `DR` vs `TDR/RDR`) change across STM32 families."],
        ["Code Size & Overhead", "Larger Flash footprint; adds function call, lock, and state-checking overhead (~10–50 cycles).", "Near-zero overhead (`static inline` compiles to 1–3 assembly instructions).", "Minimal possible Flash and CPU cycle overhead."],
        ["Safety & Pitfalls", "Built-in timeouts (`HAL_TIMEOUT`) and state tracking (`HAL_BUSY`).", "No automatic timeouts or state locks; developer must clear flags properly.", "High risk of touching reserved bits, Write-1-to-Clear (w1c) traps, or RMW races."],
        ["Best Engineering Use", "Default for 90% of application firmware, complex state machines (I²C/DMA/USB), and team codebases.", "Ultra-fast high-frequency ISRs (e.g., 50 kHz motor FOC loop) while keeping readable macros.", "Deep hardware study, cycle-critical inner loops, or silicon features not exposed by HAL."]
    ],
    col_widths=[1.3, 1.9, 1.8, 1.9]
)

# ==============================================================================
# PART 2: DMA, INTERRUPTS (NVIC), AND HARDWARE TIMERS
# ==============================================================================
add_h1("2. Part 2 — DMA, Interrupts (NVIC), and Hardware Timers", page_break=True)

add_h2("2.1 Execution Paradigms: Polling vs. Interrupt-Driven vs. Circular DMA")
add_p("A central engineering decision in embedded architecture is selecting how data moves between peripherals and SRAM. Choosing the wrong paradigm either consumes 100% of the CPU spinning on hardware status flags or causes severe interrupt latency jitter in real-time control loops.")

add_figure("diagrams/fig3_polling_irq_dma.png", "Figure 2.1 — Execution Paradigms: (1) Blocking Polling; (2) Interrupt-Driven (ISR); (3) Circular DMA with Ping-Pong Buffer Ownership Timeline.")

add_styled_table(
    "Table 2.1 — Comparative Engineering Trade-offs: Polling vs. Interrupts vs. Circular DMA",
    ["Paradigm", "CPU Overhead & Latency", "Silicon Prerequisites", "Best Practical Use Cases", "Primary Failure Mode"],
    [
        ["Blocking Polling", "100% CPU utilization while waiting; zero throughput during delays.", "None. Requires only basic peripheral clock & GPIO.", "Low-speed initialization checks (WHO_AM_I, EEPROM boot reads, 1 Hz temp).", "System lockup if hardware flags fail to clear; severe control loop jitter."],
        ["Interrupt-Driven (ISR)", "CPU preempted per byte/packet; 12–24 cycle context switch overhead per IRQ.", "NVIC enabled; peripheral IRQ unmasked; handler function.", "Asynchronous asynchronous events (UART command lines, buttons, fault lines).", "Interrupt starvation; stack overflow; priority inversion if ISR is too fat."],
        ["Circular DMA", "0% CPU utilization during transfers; CPU only handles Half/Full callbacks.", "DMA controller bus master; SRAM buffer; peripheral trigger.", "High-throughput continuous streams (ADC audio/vibration, audio I2S, display).", "Buffer overwrite if CPU misses half-buffer deadline ($T_{\\text{half}}$); cache incoherency."]
    ],
    col_widths=[1.4, 1.5, 1.3, 1.4, 1.3]
)

add_h2("2.2 ARM Cortex-M NVIC Priority Model, Preemption & ISR Rules")
add_p("The ARM Nested Vectored Interrupt Controller (NVIC) arbitrates all hardware interrupts. On Cortex-M, priority values are inverted: a lower numerical value indicates a higher preemption priority (Priority 0 is the highest priority; Priority 15 is lowest on a 4-bit priority MCU).")
add_callout("Strict ISR Engineering Rules", "1. Never call blocking APIs inside an ISR: Do NOT call `HAL_Delay()`, `printf()`, `malloc()`, or `HAL_I2C_Master_Transmit()` with long timeouts inside an interrupt handler!\n2. Keep ISRs to microseconds: Set a flag, post a pointer to an RTOS queue via `FromISR` APIs, or copy a single byte into a ring buffer.\n3. Clear the interrupt source: Failure to clear the peripheral's interrupt pending flag will cause the ISR to re-trigger infinitely the instant it exits.", "DANGER")

add_h2("2.3 DMA Controller Architecture: Channels, Streams & DMAMUX")
add_p("DMA controllers are autonomous bus masters on the Multi-Layer AHB matrix. While older families (STM32F1) use fixed DMA channels tied to specific peripherals, modern families (STM32F4/F7) use configurable Streams with multi-channel selection, and newer families (STM32G0/G4/H7/U5) utilize a DMAMUX crossbar capable of routing any peripheral trigger to any DMA channel.")

add_h2("2.4 Hardware Timers Core Architecture & Operating Modes")
add_figure("diagrams/fig4_timer_adc_dma.png", "Figure 2.2 — (A) STM32 Hardware Timer Core Architecture (PSC, CNT, ARR, CCR); (B) Deterministic Pipeline: Timer TRGO -> SAR ADC -> Circular DMA -> DSP Task.")

add_p("STM32 timers consist of four essential 16-bit or 32-bit registers:")
add_bullet("Divides the incoming timer kernel clock by `(PSC + 1)`.", "Prescaler (PSC): ")
add_bullet("The running hardware counter incrementing (up-counting) or decrementing (down-counting) on each prescaled tick.", "Counter Register (CNT): ")
add_bullet("Defines the period count. When CNT equals ARR, the counter wraps and an Update Event (UEV) is generated.", "Auto-Reload Register (ARR): ")
add_bullet("Compared against CNT to generate PWM duty cycles, or latches CNT on external input edges (Input Capture).", "Capture/Compare Registers (CCR1–CCR4): ")

add_h2("2.5 Worked Timer Mathematics & The 16-Bit Prescaler Overflow Trap")
add_formula_box(
    "Timer Frequency & Period Equations",
    [
        "f_cnt = f_TIM / (PSC + 1)",
        "f_update = f_cnt / (ARR + 1) = f_TIM / ((PSC + 1) * (ARR + 1))",
        "Duty_PWM ≈ CCR / (ARR + 1)"
    ],
    [
        "f_TIM is the timer kernel clock (respecting the APB timer x2 multiplier rule).",
        "Both PSC and ARR registers take values minus 1 because zero indicates division by 1.",
        "ARR is limited to 65,535 on 16-bit timers (TIM1, TIM3, TIM4) and 4,294,967,295 on 32-bit timers (TIM2, TIM5)."
    ]
)

add_callout("The 16-Bit Prescaler Overflow Trap", "Consider an engineer configuring STM32F103 (f_TIM = 72 MHz) for a 1 Hz timer tick. A naive calculation might try setting PSC = 71,999 to get a 1 kHz counter clock. However, PSC is a 16-bit register: its maximum allowable value is 65,535! Writing 71,999 truncates to (71999 & 0xFFFF) = 6,463, causing the timer to run more than 11 times too fast! To achieve 1 Hz on a 16-bit timer: choose PSC = 7,199 (10 kHz tick) and ARR = 9,999 (period = 10,000 ticks = 1.000 s).", "WARNING")

add_h2("2.6 Deterministic Timer TRGO -> SAR ADC -> Circular DMA Pipeline")
add_p("To sample analog signals without software timing jitter, configure a hardware timer to output a Trigger Output (TRGO) signal on every Update Event. The ADC is configured for external hardware trigger on TIMx TRGO. Each TRGO pulse triggers conversion in hardware; the ADC End-of-Conversion (EOC) signal triggers a DMA request that transfers the 12-bit sample directly from `ADC_DR` into an SRAM ping-pong buffer.")

add_h2("2.7 Ping-Pong Buffer Ownership & The Half-Transfer Deadline Rule")
add_formula_box(
    "Circular DMA Half-Transfer Deadline",
    [
        "T_full = N / F_s",
        "T_half = N / (2 * F_s)"
    ],
    [
        "N is the total number of samples allocated in the circular DMA array.",
        "F_s is the sampling frequency in Hertz.",
        "T_half is the strict real-time deadline for software to finish processing one half-buffer before the DMA controller wraps around and overwrites it."
    ]
)

add_code_block("Deterministic Timer + ADC + Circular DMA Implementation with Overrun Counter", """
#define ADC_BUF_LEN  200  /* 200 samples total: 100 samples per half-buffer */

static uint16_t adc_dma_buffer[ADC_BUF_LEN];
static volatile uint8_t g_adc_half_ready = 0;
static volatile uint8_t g_adc_full_ready = 0;
static volatile uint32_t g_adc_overruns = 0;

void DataAcquisition_Start(void)
{
    /* Start circular DMA transfer from ADC1 into adc_dma_buffer */
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_BUF_LEN) != HAL_OK) {
        Error_Handler();
    }
    /* Start hardware timer triggering ADC conversions via TRGO */
    if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        if (g_adc_half_ready) g_adc_overruns++; /* Overrun: previous half was not processed! */
        g_adc_half_ready = 1;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        if (g_adc_full_ready) g_adc_overruns++; /* Overrun: previous full was not processed! */
        g_adc_full_ready = 1;
    }
}

void DataAcquisition_Process(void)
{
    if (g_adc_half_ready) {
        g_adc_half_ready = 0;
        DSP_ProcessBuffer(&adc_dma_buffer[0], ADC_BUF_LEN / 2);
    }
    if (g_adc_full_ready) {
        g_adc_full_ready = 0;
        DSP_ProcessBuffer(&adc_dma_buffer[ADC_BUF_LEN / 2], ADC_BUF_LEN / 2);
    }
}
""")

# ==============================================================================
# PART 3: COMMUNICATION PROTOCOLS (UART, SPI, I2C, CAN, USB)
# ==============================================================================
add_h1("3. Part 3 — Serial Communication Protocols (UART, SPI, I²C, CAN, USB)", page_break=True)

add_figure("diagrams/fig5_serial_protocols.png", "Figure 3.1 — Serial Topologies & Frame Formats: (A) UART (8-N-1); (B) SPI (Full-Duplex Modes 0–3); (C) I²C (Shared Open-Drain Bus with External Pull-Ups).")

add_h2("3.1 UART Architecture: Framing, Baud Rates & Ring Buffers")
add_p("Universal Asynchronous Receiver-Transmitter (UART) is a point-to-point asynchronous protocol requiring no shared clock line. In standard 8-N-1 format, each byte consumes 10 bit-times (1 Start bit low, 8 data bits, 1 Stop bit high).")
add_formula_box(
    "UART Timing & Maximum Payload Throughput",
    [
        "t_bit = 1 / Baud",
        "Throughput_max ≈ Baud / 10  (Bytes / second)"
    ],
    [
        "At 115,200 baud, 1 bit-time = 8.68 microseconds.",
        "Maximum payload throughput is approximately 11,520 bytes/sec (11.25 KiB/s).",
        "Baud rate error between sender and receiver must remain under 2% to prevent framing errors."
    ]
)

add_code_block("Robust Line-Buffered UART Interrupt Parser with Overflow Counter", """
static uint8_t g_rx_char;
static char g_rx_line[64];
static size_t g_rx_idx = 0;
static volatile uint8_t g_cmd_ready = 0;
static volatile uint32_t g_uart_overflows = 0;

void UART_StartListening(void)
{
    HAL_UART_Receive_IT(&huart2, &g_rx_char, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        char c = (char)g_rx_char;
        if (c == '\\n') {
            g_rx_line[g_rx_idx] = '\\0';
            g_cmd_ready = 1;
            g_rx_idx = 0;
        } else if (c != '\\r') {
            if (g_rx_idx < sizeof(g_rx_line) - 1) {
                g_rx_line[g_rx_idx++] = c;
            } else {
                g_rx_idx = 0;
                g_uart_overflows++; /* Buffer overflow: input line exceeded 63 characters */
            }
        }
        HAL_UART_Receive_IT(&huart2, &g_rx_char, 1);
    }
}
""")

add_h2("3.2 SPI Architecture: Modes 0–3 & Full-Duplex Shift Operations")
add_styled_table(
    "Table 3.1 — SPI Clock Polarity (CPOL) and Clock Phase (CPHA) Mode Definitions",
    ["SPI Mode", "CPOL", "CPHA", "Clock Idle State", "Data Latch / Sample Edge", "Data Shift / Output Edge"],
    [
        ["Mode 0", "0", "0", "Logic Low (0V)", "First Edge (Rising)", "Second Edge (Falling)"],
        ["Mode 1", "0", "1", "Logic Low (0V)", "Second Edge (Falling)", "First Edge (Rising)"],
        ["Mode 2", "1", "0", "Logic High (VDD)", "First Edge (Falling)", "Second Edge (Rising)"],
        ["Mode 3", "1", "1", "Logic High (VDD)", "Second Edge (Rising)", "First Edge (Falling)"]
    ],
    col_widths=[1.2, 0.8, 0.8, 1.3, 1.4, 1.4]
)

add_code_block("SPI Full-Duplex Register Read with Dummy Clocks & Software CS", """
uint8_t SPI_ReadRegister(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg)
{
    uint8_t tx_buf[2] = { reg | 0x80, 0x00 }; /* Bit 7 = 1 for SPI Read; 0x00 dummy byte */
    uint8_t rx_buf[2] = { 0, 0 };

    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_RESET); /* Assert Chip Select (Active Low) */
    HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 2, 50);
    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_SET);   /* De-assert Chip Select */

    return rx_buf[1]; /* Second byte latched the sensor's register data */
}
""")

add_h2("3.3 I²C Architecture: Open-Drain Physics, Pull-Ups & 9-Clock Recovery")
add_formula_box(
    "I²C Bus Rise Time & Pull-Up Calculation",
    [
        "t_r ≈ 0.847 * R_pu * C_bus",
        "R_pu_min = (VDD - V_OL_max) / I_OL",
        "R_pu_max = t_r_max / (0.847 * C_bus)"
    ],
    [
        "For Fast-mode (400 kHz), maximum allowable rise time t_r_max = 300 ns.",
        "With C_bus ≈ 100 pF, R_pu should typically range between 2.2 kOhm and 4.7 kOhm.",
        "Weak internal MCU pull-ups (~40 kOhm) produce t_r > 3.3 microseconds, causing severe I²C bus failure at 400 kHz!"
    ]
)

add_callout("The 7-Bit Address Left-Shift Rule & 9-Clock Recovery", "1. STM32 HAL Expects Shifted Addresses: In `HAL_I2C_Mem_Read()` and `HAL_I2C_Master_Transmit()`, ST expects the 7-bit device address shifted left by 1 bit: `DevAddress = (ADDR_7BIT << 1)`. For MPU6050 (0x68), pass `(0x68 << 1) = 0xD0`.\n2. 9-Clock Bus Lockup Recovery: If an MCU resets while an I²C slave is transmitting a logic 0, SDA will remain locked low forever. To recover: configure SCL as GPIO output, toggle SCL up to 9 times until SDA is released high, issue a STOP condition, and re-initialize the I²C peripheral.", "TIP")

add_h2("3.4 Protocol Selection Matrix & Real-World System Scenarios")
add_styled_table(
    "Table 3.2 — Peripheral Selection Matrix Across Five Real-World Engineering Scenarios",
    ["Scenario / Application", "Recommended Protocol", "Key Engineering Rationale", "Alternative Considered & Why Rejected"],
    [
        ["Scenario A: High-Speed 6-Axis IMU (1 kHz) on Robot Chassis", "SPI (5–10 MHz)", "High bandwidth; deterministic latency; hardware CS prevents bus collisions.", "I²C rejected: 400 kHz I²C takes ~250 µs per read, consuming 25% of the 1 ms time budget."],
        ["Scenario B: Board Monitoring (Temp, Pressure, EEPROM)", "I²C (400 kHz)", "Multi-device shared bus saves MCU pins; low data rate (<10 Hz).", "SPI rejected: Would require 3 separate Chip-Select GPIO lines, wasting pin budget."],
        ["Scenario C: Telemetry to Host PC or Bluetooth Module", "UART (115.2k or 921.6k)", "Standard asynchronous point-to-point interface; native PC VCP support.", "SPI/I²C rejected: Host PC operating systems lack native exposed SPI/I²C master controllers."],
        ["Scenario D: Multi-Node Robotics CAN Bus Network", "CAN 2.0B (1 Mbps)", "Differential noise immunity; hardware arbitration; multi-master IDs.", "UART/RS-485 rejected: Lacks built-in hardware message priority arbitration and CRC validation."],
        ["Scenario E: High-Throughput Waveform Streaming", "USB Full-Speed CDC (12 Mbps)", "Carries 1 MB/s continuous streaming data directly into PC analysis software.", "UART rejected: 115,200 baud tops out at ~11 KiB/s, far too slow for raw waveform streaming."]
    ],
    col_widths=[1.5, 1.2, 2.3, 1.9]
)

# ==============================================================================
# PART 4: SENSORS, ADC, REAL-TIME ACQUISITION & DSP
# ==============================================================================
add_h1("4. Part 4 — Sensors, ADC, Real-Time Acquisition & Digital Signal Processing", page_break=True)

add_h2("4.1 SAR ADC Architecture, Sampling Time & Source Impedance")
add_formula_box(
    "SAR ADC Transfer Function & Conversion Time",
    [
        "V_in = Code * (V_REF / (2^N - 1))",
        "T_conv = T_sample + 12.5 ADC_Cycles"
    ],
    [
        "N is the ADC resolution (typically 12 bits -> 4,095 full scale).",
        "If V_REF is tied to an unfiltered 3.3V digital rail, power supply ripple directly pollutes conversions.",
        "The internal sampling capacitor (C_sample ≈ 4–8 pF) requires sufficient sample time to settle through source impedance R_source."
    ]
)

add_callout("ADC Source Impedance & Channel Crosstalk Rule", "If your analog sensor or resistor divider has a high source impedance (R_source > 10 kOhm), the ADC sampling capacitor will NOT fully charge during short sampling windows (e.g., 1.5 or 7.5 cycles). As the ADC multiplexer switches channels, residual charge from the previous channel will bleed into the current channel, causing severe crosstalk and reading errors! Solution: Select a sampling time of 71.5 to 239.5 cycles in CubeMX, or add an operational amplifier buffer stage.", "WARNING")

add_h2("4.2 Analog Front-End (AFE) Conditioning & NTC Steinhart-Hart Equation")
add_p("To convert an NTC thermistor's non-linear resistance into accurate engineering units (°C), use the Steinhart-Hart equation or the simplified Beta parameter formula:")
add_formula_box(
    "NTC Thermistor Beta Formulation",
    [
        "1 / T_Kelvin = 1 / T0 + (1 / Beta) * ln(R_ntc / R0)",
        "T_Celsius = T_Kelvin - 273.15"
    ],
    [
        "R0 is the rated thermistor resistance at reference temperature T0 (usually 10 kOhm at 298.15 K / 25 °C).",
        "Beta is the manufacturer material constant (typically 3,435 K to 3,950 K).",
        "Never approximate an NTC thermistor as a linear voltage divider across a wide operating range."
    ]
)

add_h2("4.3 Digital Signal Processing (DSP): Moving Average & EMA Filters")
add_styled_table(
    "Table 4.1 — Digital Filter Comparison: Moving Average (FIR) vs. Exponential Moving Average (IIR)",
    ["Filter Type", "Mathematical Formula", "Memory Footprint", "Computational Complexity", "Best Engineering Application"],
    [
        ["Moving Average (Boxcar FIR)", "y[n] = (1/K) * sum(x[n - i])", "Requires array of K history samples.", "Moderate: ring buffer index management & sum subtraction.", "Smoothing periodic sensor noise where sharp step response is not required."],
        ["Exponential Moving Average (EMA IIR)", "y[n] = alpha * x[n] + (1 - alpha) * y[n - 1]", "Requires only 1 state variable (y[n-1]).", "Minimal: 1 multiply-accumulate; extremely fast.", "Real-time microcontroller filtering; low-pass telemetry filtering with zero memory overhead."]
    ],
    col_widths=[1.5, 1.8, 1.4, 1.1, 1.1]
)

add_code_block("Efficient Fixed-Point Exponential Moving Average (EMA) Filter in C", """
typedef struct {
    uint32_t alpha_q16; /* Smoothing factor in Q16 fixed-point (0 to 65536) */
    int32_t  state_q16; /* Filter state variable */
} EmaFilter;

void EMA_Init(EmaFilter *f, float alpha)
{
    if (alpha > 1.0f) alpha = 1.0f;
    if (alpha < 0.0f) alpha = 0.0f;
    f->alpha_q16 = (uint32_t)(alpha * 65536.0f);
    f->state_q16 = 0;
}

float EMA_Update(EmaFilter *f, float input)
{
    int32_t input_q16 = (int32_t)(input * 65536.0f);
    f->state_q16 = f->state_q16 + (int32_t)(((int64_t)f->alpha_q16 * (input_q16 - f->state_q16)) >> 16);
    return (float)f->state_q16 / 65536.0f;
}
""")

# ==============================================================================
# PART 5: REUSABLE SENSOR DRIVERS & REUSABLE EMBEDDED SOFTWARE
# ==============================================================================
add_h1("5. Part 5 — Reusable Sensor Drivers & Modular Software Architecture", page_break=True)

add_figure("diagrams/fig6_driver_and_rtos.png", "Figure 5.1 — (A) 4-Layer Reusable Sensor Driver Architecture; (B) FreeRTOS Multi-Sensor Monitoring Architecture with Queues and Mutexes.")

add_h2("5.1 The 4-Layer Sensor Driver Architecture")
add_p("Professional embedded software decouples hardware register mechanics from application control policy through four distinct layers (see Figure 5.1A):")
add_bullet("Owns thresholds, state machines, alarms, and control decisions. Zero knowledge of peripheral registers or I2C addresses.", "1. Application Layer (app.c): ")
add_bullet("Owns register maps, signed two's complement decoding, unit conversion (to SI units), and status enums.", "2. Device Driver Layer (mpu6050.c): ")
add_bullet("Injected handle pointer or function-pointer IO table enabling bus timeouts, thread-safe mutex locking, and host-PC mocking.", "3. Bus Transport Interface: ")
add_bullet("Generated peripheral initialization, GPIO Alternate Functions, NVIC routing, and DMA linking.", "4. CubeMX Hardware & HAL MSP: ")

add_h2("5.2 Ten Golden Rules for Production Sensor Driver APIs")
add_bullet("Return a rich status enum (`SENSOR_OK`, `SENSOR_ERR_BUS`, `SENSOR_ERR_TIMEOUT`), never a bare boolean.", "Rule 1: Typed Status Enums — ")
add_bullet("Return measurements via output pointers; NEVER modify output data unless the read was completely successful.", "Rule 2: Validated Output Pointers — ")
add_bullet("Encode SI units directly into function and variable identifiers (`temp_celsius`, `accel_mps2`, `rate_radps`).", "Rule 3: Explicit Physical Units — ")
add_bullet("Every bus transaction MUST specify a finite timeout; never block indefinitely waiting on hardware flags.", "Rule 4: Bounded Timeouts — ")
add_bullet("Pass peripheral handles via pointer parameters to support multiple sensor instances on the same bus.", "Rule 5: Dependency Injection — ")
add_bullet("Verify WHO_AM_I device ID during initialization before configuring operational registers.", "Rule 6: Silicon Identification — ")
add_bullet("Sign-extend two's complement 12-bit, 14-bit, or 16-bit register pairs using proper typed casts.", "Rule 7: Sign Extension Discipline — ")
add_bullet("Never call `malloc()` inside sensor drivers; keep all state in caller-allocated structures.", "Rule 8: Deterministic Memory Allocation — ")
add_bullet("Wrap shared serial buses in mutexes or critical sections when accessing from multiple tasks.", "Rule 9: Thread Safety — ")
add_bullet("Structure drivers so bus calls can be stubbed out to run unit tests on host PC development platforms.", "Rule 10: Host Mocking & Testability — ")

add_h2("5.3 Production MPU6050 6-Axis IMU Driver Implementation")
add_code_block("mpu6050.h — Header Interface with Dependency Injection & Status Enums", """
#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define MPU6050_I2C_ADDR_DEFAULT  (0x68 << 1)
#define MPU6050_WHO_AM_I_REG      0x75
#define MPU6050_WHO_AM_I_VAL      0x68
#define MPU6050_PWR_MGMT_1_REG    0x6B
#define MPU6050_ACCEL_XOUT_H_REG  0x3B

typedef enum {
    MPU6050_OK = 0,
    MPU6050_ERR_ARG,
    MPU6050_ERR_BUS,
    MPU6050_ERR_WRONG_ID,
    MPU6050_ERR_TIMEOUT
} MpuStatus;

typedef struct {
    float accel_x_g, accel_y_g, accel_z_g;
    float gyro_x_dps, gyro_y_dps, gyro_z_dps;
    float temp_c;
} MpuData;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t dev_addr_hal;
    uint8_t is_initialized;
} MpuDevice;

MpuStatus MPU6050_Init(MpuDevice *dev, I2C_HandleTypeDef *hi2c, uint16_t addr_hal);
MpuStatus MPU6050_ReadSensors(MpuDevice *dev, MpuData *out_data);

#endif
""")

add_code_block("mpu6050.c — Burst Register Read, Two's Complement Decoding & Scaling", """
#include "mpu6050.h"

MpuStatus MPU6050_Init(MpuDevice *dev, I2C_HandleTypeDef *hi2c, uint16_t addr_hal)
{
    uint8_t who_am_i = 0;
    if (!dev || !hi2c) return MPU6050_ERR_ARG;

    dev->hi2c = hi2c;
    dev->dev_addr_hal = addr_hal;
    dev->is_initialized = 0;

    /* Verify silicon identity */
    if (HAL_I2C_Mem_Read(dev->hi2c, dev->dev_addr_hal, MPU6050_WHO_AM_I_REG,
                         I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, 100) != HAL_OK) {
        return MPU6050_ERR_BUS;
    }
    if (who_am_i != MPU6050_WHO_AM_I_VAL) {
        return MPU6050_ERR_WRONG_ID;
    }

    /* Wake device from sleep (clear SLEEP bit in PWR_MGMT_1) */
    uint8_t pwr_wake = 0x00;
    if (HAL_I2C_Mem_Write(dev->hi2c, dev->dev_addr_hal, MPU6050_PWR_MGMT_1_REG,
                          I2C_MEMADD_SIZE_8BIT, &pwr_wake, 1, 100) != HAL_OK) {
        return MPU6050_ERR_BUS;
    }

    dev->is_initialized = 1;
    return MPU6050_OK;
}

MpuStatus MPU6050_ReadSensors(MpuDevice *dev, MpuData *out_data)
{
    uint8_t raw[14];
    if (!dev || !out_data || !dev->is_initialized) return MPU6050_ERR_ARG;

    /* Burst read 14 registers: Accel (6) + Temp (2) + Gyro (6) */
    if (HAL_I2C_Mem_Read(dev->hi2c, dev->dev_addr_hal, MPU6050_ACCEL_XOUT_H_REG,
                         I2C_MEMADD_SIZE_8BIT, raw, 14, 50) != HAL_OK) {
        return MPU6050_ERR_BUS;
    }

    int16_t ax_raw = (int16_t)((raw[0] << 8) | raw[1]);
    int16_t ay_raw = (int16_t)((raw[2] << 8) | raw[3]);
    int16_t az_raw = (int16_t)((raw[4] << 8) | raw[5]);
    int16_t t_raw  = (int16_t)((raw[6] << 8) | raw[7]);
    int16_t gx_raw = (int16_t)((raw[8] << 8) | raw[9]);
    int16_t gy_raw = (int16_t)((raw[10] << 8) | raw[11]);
    int16_t gz_raw = (int16_t)((raw[12] << 8) | raw[13]);

    /* Scale raw 16-bit integer codes to physical SI units per datasheet */
    out_data->accel_x_g = (float)ax_raw / 16384.0f; /* +/-2g range */
    out_data->accel_y_g = (float)ay_raw / 16384.0f;
    out_data->accel_z_g = (float)az_raw / 16384.0f;
    out_data->temp_c    = ((float)t_raw / 340.0f) + 36.53f;
    out_data->gyro_x_dps = (float)gx_raw / 131.0f;   /* +/-250 dps range */
    out_data->gyro_y_dps = (float)gy_raw / 131.0f;
    out_data->gyro_z_dps = (float)gz_raw / 131.0f;

    return MPU6050_OK;
}
""")

# ==============================================================================
# PART 6: REAL-TIME OPERATING SYSTEMS (RTOS) & FREERTOS
# ==============================================================================
add_h1("6. Part 6 — Real-Time Operating Systems (RTOS) & FreeRTOS Architecture", page_break=True)

add_h2("6.1 Superloop vs. Non-Blocking State Machine vs. Preemptive RTOS")
add_styled_table(
    "Table 6.1 — Architecture Comparison: Superloop vs. State Machine vs. FreeRTOS",
    ["Architecture", "Execution Model", "Responsiveness & Deadlines", "Memory Footprint", "When to Use"],
    [
        ["Bare-Metal Superloop", "Sequential `while(1)` polling functions.", "Poor if any function blocks; execution time of loop sums.", "Minimal (<1 KB RAM, single stack).", "Simple single-rate projects (LED blinks, basic menu)."],
        ["Non-Blocking FSM", "Single loop executing event-driven state machines with tick timers.", "Good: tasks yield immediately if hardware not ready.", "Low (single stack, small state structs).", "Medium complexity firmware with strict RAM limits."],
        ["Preemptive RTOS (FreeRTOS)", "Priority-based preemptive scheduling across independent tasks.", "Deterministic: high-priority task preempts immediately on event.", "Higher (requires dedicated stack per task + kernel heap).", "Complex multi-rate systems (e.g. 1 kHz control + USB + GUI + telemetry)."]
    ],
    col_widths=[1.4, 1.6, 1.5, 1.1, 1.3]
)

add_h2("6.2 FreeRTOS Core Objects: Tasks, Queues, Mutexes & Notifications")
add_bullet("Independent thread of execution with its own stack and assigned priority.", "Task: ")
add_bullet("Thread-safe FIFO data pipeline copying data by value from producer to consumer.", "Queue: ")
add_bullet("Mutual exclusion token with Priority Inheritance to eliminate priority inversion.", "Mutex: ")
add_bullet("Ultra-lightweight direct-to-task signal bypassing queue overhead for unblocking tasks from ISRs.", "Task Notification: ")

add_callout("The configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY Trap", "On Cortex-M with FreeRTOS, any Interrupt Service Routine (ISR) that calls a FreeRTOS API function ending in `...FromISR()` MUST have its hardware NVIC priority configured numerically GREATER THAN OR EQUAL TO `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (which means logically lower or equal priority). If an interrupt with a higher priority (e.g. Priority 0 to 4) calls a FreeRTOS API, the FreeRTOS critical section cannot mask it, corrupting internal kernel lists and causing an immediate HardFault!", "DANGER")

# ==============================================================================
# PART 7: CAPSTONE, DEBUGGING PLAYBOOK, LABS & KNOWLEDGE CHECK
# ==============================================================================
add_h1("7. Part 7 — Capstone System, Debugging Playbook, Labs & Reference Sheet", page_break=True)

add_h2("7.1 10-Layer Debugging Playbook")
add_styled_table(
    "Table 7.1 — Layered Debugging Playbook & Root Cause Checklist",
    ["Layer", "Observed Fault Symptom", "Immediate Silicon & Tool Verification Checks"],
    [
        ["1. Power & Ground", "Target MCU completely unresponsive; debugger cannot connect.", "Measure 3.3V VDD and VSS continuity with DMM; check NRST pin high; check BOOT0 grounded."],
        ["2. Reset & Clocks", "Program traps in `Error_Handler` before reaching `main()` loop.", "Inspect `SystemClock_Config()`; confirm external crystal (HSE) frequency matches .ioc setting."],
        ["3. Pin Multiplexing", "Peripheral signals absent on oscilloscope; inputs float.", "Verify Alternate Function (AF) mapping in datasheet; ensure ADC pin is in GPIO_MODE_ANALOG."],
        ["4. Peripheral Engine", "UART transmits illegible characters; I²C devices NACK.", "Scope TX bit-time (measure baud error); verify I²C pull-up resistors and 7-bit left-shift."],
        ["5. Data Movement", "DMA Half/Full callbacks never fire; buffers contain zeroes.", "Verify DMA controller clock enabled; confirm request stream/channel matches RM mapping."],
        ["6. Interrupt Controller", "Peripheral status flag asserts in SFR view but ISR never executes.", "Verify NVIC IRQ is unmasked in CubeMX NVIC Configuration; check NVIC Priority Grouping."],
        ["7. Buffer Ownership", "Intermittent torn readings or erratic sensor glitches.", "Audit shared variables for `volatile` keyword; verify ping-pong half-buffer processing deadlines."],
        ["8. Driver & Scaling", "Sensor outputs constant 0x00 or garbage physical values.", "Check WHO_AM_I return code; verify signed two's complement decoding and datasheet LSB scale."],
        ["9. Application Policy", "System deadlocks when a sensor wire is unplugged.", "Replace unbounded while-loops with finite timeouts; add bus reset and reconnect state machines."],
        ["10. Telemetry & Link", "High-frequency control loop runs slowly with severe jitter.", "Remove blocking `HAL_UART_Transmit` calls from control loops; stream packets via non-blocking DMA."]
    ],
    col_widths=[1.3, 2.3, 3.3]
)

add_h2("7.2 HardFault Forensics on ARM Cortex-M")
add_p("When execution traps inside `HardFault_Handler()`, the ARM Cortex-M core automatically pushes an exception stack frame onto the active stack (MSP or PSP). Inspecting Special Function Registers (SFRs) reveals the exact failure point:")
add_bullet("Contains the Program Counter address of the exact machine instruction that triggered the fault.", "Stacked PC: ")
add_bullet("Configurable Fault Status Register detailing Instruction Access Violation (`IACCVIOL`), Data Access Violation (`DACCVIOL`), Undefined Instruction (`UNDEFINSTR`), or Divide-by-Zero (`DIVBYZERO`).", "CFSR: ")
add_bullet("Bus Fault Address Register holding the physical memory address that caused a bus error (valid if `BFARVALID` bit is set).", "BFAR: ")
add_bullet("Memory Management Fault Address Register holding the address that violated MPU permissions (valid if `MMFARVALID` bit is set).", "MMFAR: ")

add_h2("7.3 Seven Structured Engineering Labs")
labs_data = [
    ("Lab 1: GPIO Heartbeat & Hardware Clock Proof", "Configure User LED push-pull output; toggle with 250 ms delay; verify reset vector and SYSCLK stability.", "Visible LED blinking after power-on without debugger attached."),
    ("Lab 2: Non-Blocking UART Console & Overflow Handling", "Implement bidirectional framed line receiver with ring buffer and overflow counter.", "Receiving 100-byte line increments overflow counter without locking up system."),
    ("Lab 3: Hardware Timer Calculation & Scope Measurement", "Calculate PSC and ARR for 1 kHz output; output square wave on GPIO pin.", "Measured frequency on oscilloscope matches formula within crystal tolerance."),
    ("Lab 4: Analog Front-End & SAR ADC Polling", "Read 12-bit analog input across 0V, mid-rail, and 3.3V with resistive divider.", "ADC integer code tracks applied voltage monotonically ($V = \\text{code} \\times 3.3 / 4095$)."),
    ("Lab 5: Deterministic Timer-Triggered ADC + Circular DMA", "Configure TIM2 TRGO triggering ADC1 with circular DMA; count buffer overruns.", "Zero buffer overruns observed while streaming samples at 10 kS/s."),
    ("Lab 6: Reusable Sensor Driver with Failure Detection", "Implement modular MPU6050 driver returning typed status enums; test unplug recovery.", "Unplugging I2C cable returns `MPU6050_ERR_BUS` without crashing application."),
    ("Lab 7: FreeRTOS Real-Time Producer-Consumer System", "Create acquisition task, telemetry task, and queue; monitor task stack high-watermarks.", "Tasks operate deterministically; queue full policy drops oldest sample safely.")
]
for title, desc, accept in labs_data:
    add_h3(title)
    add_bullet(desc, "Objective & Workflow: ")
    add_bullet(accept, "Strict Acceptance Criteria: ")

add_h2("7.4 Master Formula Quick-Reference Sheet")
add_styled_table(
    "Table 7.2 — Master Formula Reference Sheet for Laboratory & Field Calculations",
    ["Physical Quantity", "Standard Formula", "Key Parameters & Silicon Constraints"],
    [
        ["Timer Update Frequency", "f_update = f_TIM / ((PSC + 1) * (ARR + 1))", "Check APB timer x2 multiplier rule in RCC clock tree."],
        ["Edge-Aligned PWM Duty", "Duty ≈ CCR / (ARR + 1)", "Valid for PWM Mode 1 active-high up-counting."],
        ["ADC Voltage Conversion", "V_in = Code * (V_REF / (2^N - 1))", "N = 12 bits -> 4,095 full scale. V_REF accuracy dominates."],
        ["DMA Half-Transfer Period", "T_half = N / (2 * F_s)", "Processing time MUST remain strictly below T_half to prevent overwrites."],
        ["UART Payload Capacity", "Bytes/sec ≈ Baud / 10", "Applies to 8-N-1 format (10 bit-times per byte)."],
        ["UART Bit Duration", "t_bit = 1 / Baud", "At 115,200 baud, t_bit = 8.68 microseconds."],
        ["I²C Line Rise Time", "t_r ≈ 0.847 * R_pu * C_bus", "Must remain < 300 ns for 400 kHz Fast-mode operation."],
        ["Stream Bandwidth", "Bandwidth = F_s * Bytes * Channels", "Raw bytes generated before packet framing or compression."]
    ],
    col_widths=[1.8, 2.3, 2.8]
)

# ------------------------------------------------------------------------------
# SAVE DOCUMENT
# ------------------------------------------------------------------------------
doc.save(OUTPUT_DOCX_PATH)
file_size_kb = os.path.getsize(OUTPUT_DOCX_PATH) / 1024.0
print(f"Master document generated successfully: {OUTPUT_DOCX_PATH} ({file_size_kb:.1f} KB)")
