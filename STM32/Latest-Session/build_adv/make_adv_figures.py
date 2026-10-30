import os
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

OUTPUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "diagrams_adv"))
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Visual Identity Palette
COLOR_NAVY_PRIMARY = "#0F2537"
COLOR_BLUE_SECONDARY = "#1E4E79"
COLOR_TEAL_ACCENT = "#008080"
COLOR_LIGHT_BG = "#F8FAFC"
COLOR_BORDER_GRAY = "#D0D7DE"
COLOR_MUTED_GRAY = "#666666"
COLOR_TEXT_DARK = "#222222"
COLOR_ALERT_ORANGE = "#D97706"
COLOR_ALERT_RED = "#B91C1C"
COLOR_GREEN = "#15803D"

def save_fig(fig, filename):
    path = os.path.join(OUTPUT_DIR, filename)
    fig.savefig(path, dpi=300, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {path}")

# =========================================================================
# FIGURE 1: Flash Architecture & EEPROM Emulation
# =========================================================================
def make_fig1():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.patch.set_facecolor("white")
    
    # Subplot A: Flash Memory Organization (F1 Pages vs F4 Sectors)
    ax1.set_facecolor(COLOR_LIGHT_BG)
    ax1.set_xlim(0, 10)
    ax1.set_ylim(0, 10)
    ax1.axis("off")
    ax1.set_title("(A) Internal Flash Organization: F103 vs. F405", fontsize=12, fontweight="bold", color=COLOR_NAVY_PRIMARY, pad=12)
    
    # F1 Column
    ax1.text(2.5, 9.2, "STM32F103 (Uniform 1 KB Pages)", fontsize=10, fontweight="bold", ha="center", color=COLOR_BLUE_SECONDARY)
    f1_boxes = [("Page 0 (0x0800 0000) [1 KB]", "#E2E8F0"),
                ("Page 1 (0x0800 0400) [1 KB]", "#E2E8F0"),
                ("...", "#CBD5E1"),
                ("Page 62 (0x0800 F800) [1 KB]", "#BAE6FD"),
                ("Page 63 (0x0800 FC00) [1 KB] -> EEPROM", "#7DD3FC")]
    y = 8.2
    for name, col in f1_boxes:
        rect = patches.FancyBboxPatch((0.5, y), 4.0, 0.9, boxstyle="round,pad=0.05", facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, linewidth=1.2)
        ax1.add_patch(rect)
        ax1.text(2.5, y + 0.45, name, fontsize=8.5, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")
        y -= 1.3
        
    # F4 Column
    ax1.text(7.5, 9.2, "STM32F405 (Asymmetric Sectors)", fontsize=10, fontweight="bold", ha="center", color=COLOR_BLUE_SECONDARY)
    f4_boxes = [("Sector 0 (0x0800 0000) [16 KB]", "#E2E8F0"),
                ("Sector 1..3 (16 KB each)", "#E2E8F0"),
                ("Sector 4 (64 KB)", "#E2E8F0"),
                ("Sector 5..11 (128 KB each)", "#CBD5E1"),
                ("Sector 11 (0x080E 0000) [128 KB]", "#FED7AA")]
    y = 8.2
    for name, col in f4_boxes:
        rect = patches.FancyBboxPatch((5.5, y), 4.0, 0.9, boxstyle="round,pad=0.05", facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, linewidth=1.2)
        ax1.add_patch(rect)
        ax1.text(7.5, y + 0.45, name, fontsize=8.5, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")
        y -= 1.3
        
    ax1.text(5.0, 0.8, "Erase Granularity: F1 = 1 KB page | F4 = 16 KB to 128 KB sector!\nProgramming Width: F1 = 16-bit HalfWord | F4 = 8/16/32/64-bit", 
             fontsize=9, ha="center", style="italic", bbox=dict(boxstyle="square,pad=0.4", facecolor="white", edgecolor=COLOR_TEAL_ACCENT))

    # Subplot B: EEPROM Emulation Two-Page Ping-Pong Swapping
    ax2.set_facecolor(COLOR_LIGHT_BG)
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 10)
    ax2.axis("off")
    ax2.set_title("(B) Dual-Page Wear-Leveling Swapping Protocol", fontsize=12, fontweight="bold", color=COLOR_NAVY_PRIMARY, pad=12)
    
    # Page A (Active / Filling)
    rect_a = patches.FancyBboxPatch((0.8, 4.0), 3.8, 5.0, boxstyle="round,pad=0.1", facecolor="#F0FDF4", edgecolor=COLOR_GREEN, linewidth=1.8)
    ax2.add_patch(rect_a)
    ax2.text(2.7, 8.5, "PAGE A (STATUS: VALID)", fontsize=10, fontweight="bold", ha="center", color=COLOR_GREEN)
    records = ["Header: 0x55AA (VALID)", "Var 0x0001 = 120 (PID_P)", "Var 0x0002 = 45  (PID_I)", "Var 0x0001 = 135 (UPDATED)", "... Sequential Records ..."]
    ry = 7.7
    for rec in records:
        rbox = patches.Rectangle((1.0, ry), 3.4, 0.6, facecolor="white", edgecolor=COLOR_BORDER_GRAY)
        ax2.add_patch(rbox)
        ax2.text(2.7, ry + 0.3, rec, fontsize=7.5, ha="center", va="center", color=COLOR_TEXT_DARK)
        ry -= 0.8

    # Page B (Erased / Ready for Swap)
    rect_b = patches.FancyBboxPatch((5.4, 4.0), 3.8, 5.0, boxstyle="round,pad=0.1", facecolor="#F8FAFC", edgecolor=COLOR_BORDER_GRAY, linewidth=1.8)
    ax2.add_patch(rect_b)
    ax2.text(7.3, 8.5, "PAGE B (STATUS: ERASED)", fontsize=10, fontweight="bold", ha="center", color=COLOR_MUTED_GRAY)
    ax2.text(7.3, 6.5, "[ All Words = 0xFFFF ]\n\nReady to receive packed\nvalid variables when\nPAGE A fills up", 
             fontsize=8.5, ha="center", va="center", color=COLOR_MUTED_GRAY)
    
    # Transfer Arrow
    ax2.annotate("1. Page A Full\n2. Copy latest vars to B\n3. Mark B VALID\n4. Erase A to ERASED", 
                 xy=(5.4, 4.8), xytext=(2.7, 2.2),
                 arrowprops=dict(facecolor=COLOR_TEAL_ACCENT, shrink=0.08, width=2, headwidth=8),
                 fontsize=8.5, fontweight="bold", color=COLOR_NAVY_PRIMARY,
                 bbox=dict(boxstyle="round,pad=0.4", facecolor="#FEF3C7", edgecolor=COLOR_ALERT_ORANGE))
    
    plt.tight_layout()
    save_fig(fig, "fig1_flash_and_eeprom.png")

# =========================================================================
# FIGURE 2: Quadrature Encoder Signals & Timer Decoding
# =========================================================================
def make_fig2():
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(13, 7), gridspec_kw={'height_ratios': [1.2, 1]})
    fig.patch.set_facecolor("white")
    
    # Waveforms
    ax1.set_facecolor(COLOR_LIGHT_BG)
    ax1.set_xlim(0, 16)
    ax1.set_ylim(-0.5, 3.5)
    ax1.axis("off")
    ax1.set_title("(A) Quadrature Waveforms (90° Phase Shift) & Edge Counting Modes", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    # Time vector
    t = np.linspace(0, 16, 1000)
    
    # Forward rotation (0 to 8): Ch A leads Ch B
    ax1.text(0.2, 2.8, "FORWARD ROTATION (Clockwise): Phase A leads Phase B by 90°", fontsize=9, fontweight="bold", color=COLOR_BLUE_SECONDARY)
    # Channel A
    # Square wave: 0-1 high, 1-2 low...
    ax1.plot([0, 1, 1, 3, 3, 5, 5, 7, 7, 8], [2.2, 2.2, 1.4, 1.4, 2.2, 2.2, 1.4, 1.4, 2.2, 2.2], color=COLOR_NAVY_PRIMARY, lw=2)
    ax1.text(-0.8, 1.8, "CH A\n(TI1)", fontsize=9, fontweight="bold", color=COLOR_NAVY_PRIMARY, va="center")
    # Channel B (lags A by 0.5 unit = 90 deg)
    ax1.plot([0, 0.5, 0.5, 2.5, 2.5, 4.5, 4.5, 6.5, 6.5, 8], [0.4, 0.4, 1.2, 1.2, 0.4, 0.4, 1.2, 1.2, 0.4, 0.4], color=COLOR_TEAL_ACCENT, lw=2)
    ax1.text(-0.8, 0.8, "CH B\n(TI2)", fontsize=9, fontweight="bold", color=COLOR_TEAL_ACCENT, va="center")
    
    # Reverse rotation (8 to 16): Ch B leads Ch A
    ax1.text(8.2, 2.8, "REVERSE ROTATION (Counter-Clockwise): Phase B leads Phase A by 90°", fontsize=9, fontweight="bold", color=COLOR_ALERT_ORANGE)
    ax1.plot([8, 9, 9, 11, 11, 13, 13, 15, 15, 16], [2.2, 2.2, 1.4, 1.4, 2.2, 2.2, 1.4, 1.4, 2.2, 2.2], color=COLOR_NAVY_PRIMARY, lw=2)
    ax1.plot([8, 8.5, 8.5, 10.5, 10.5, 12.5, 12.5, 14.5, 14.5, 16], [1.2, 1.2, 0.4, 0.4, 1.2, 1.2, 0.4, 0.4, 1.2, 1.2], color=COLOR_TEAL_ACCENT, lw=2)
    
    # 4x count ticks
    for x in np.arange(0.5, 8, 0.5):
        ax1.axvline(x, color="gray", linestyle=":", alpha=0.6)
        ax1.text(x, -0.2, "+1", fontsize=7.5, color=COLOR_GREEN, ha="center", fontweight="bold")
    for x in np.arange(8.5, 16, 0.5):
        ax1.axvline(x, color="gray", linestyle=":", alpha=0.6)
        ax1.text(x, -0.2, "-1", fontsize=7.5, color=COLOR_ALERT_RED, ha="center", fontweight="bold")

    # Subplot B: Timer Hardware Block Diagram
    ax2.set_facecolor("white")
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 5)
    ax2.axis("off")
    ax2.set_title("(B) STM32 Timer Hardware Encoder Interface (TI1FP1 & TI2FP2)", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    blocks = [
        ("Input Pin CH1 (PA0)", (0.5, 3.2), (1.8, 1.0), "#E2E8F0"),
        ("Input Pin CH2 (PA1)", (0.5, 0.8), (1.8, 1.0), "#E2E8F0"),
        ("Input Filter & Polarity (IC1F / CC1P)", (2.8, 3.2), (2.4, 1.0), "#BAE6FD"),
        ("Input Filter & Polarity (IC2F / CC2P)", (2.8, 0.8), (2.4, 1.0), "#BAE6FD"),
        ("Encoder State Decoder\n(SMS[2:0] = 011 -> Mode 3 x4)", (5.8, 1.8), (2.2, 1.8), "#7DD3FC"),
        ("Counter Register (CNT)\nAuto-Reload (ARR)", (8.4, 1.8), (1.4, 1.8), "#BBF7D0"),
    ]
    for text, (bx, by), (bw, bh), col in blocks:
        rect = patches.FancyBboxPatch((bx, by), bw, bh, boxstyle="round,pad=0.08", facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, lw=1.2)
        ax2.add_patch(rect)
        ax2.text(bx + bw/2, by + bh/2, text, fontsize=7.5, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")
        
    # Arrows
    arrows = [((2.3, 3.7), (2.8, 3.7)), ((2.3, 1.3), (2.8, 1.3)),
              ((5.2, 3.7), (5.8, 3.0)), ((5.2, 1.3), (5.8, 2.4)),
              ((8.0, 2.7), (8.4, 2.7))]
    for start, end in arrows:
        ax2.annotate("", xy=end, xytext=start, arrowprops=dict(facecolor=COLOR_NAVY_PRIMARY, shrink=0.05, width=1.5, headwidth=6))
        
    ax2.text(8.2, 3.2, "Clock / Dir", fontsize=7.5, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    plt.tight_layout()
    save_fig(fig, "fig2_quadrature_encoder.png")

# =========================================================================
# FIGURE 3: Low Power Modes State Machine & Current Profile
# =========================================================================
def make_fig3():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.patch.set_facecolor("white")
    
    # State diagram
    ax1.set_facecolor(COLOR_LIGHT_BG)
    ax1.set_xlim(0, 10)
    ax1.set_ylim(0, 10)
    ax1.axis("off")
    ax1.set_title("(A) STM32 Low-Power Operating Modes & Transitions", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    # Mode circles
    modes = [
        ("RUN MODE\nCore ON | Clocks ON\n~30-50 mA", (5, 8.5), "#BAE6FD", COLOR_BLUE_SECONDARY),
        ("SLEEP MODE\nCore OFF | Peripherals ON\n~10-15 mA", (2, 5.0), "#BBF7D0", COLOR_GREEN),
        ("STOP MODE\nClocks OFF | SRAM Retained\n~10-25 µA", (5, 1.8), "#FED7AA", COLOR_ALERT_ORANGE),
        ("STANDBY MODE\nSRAM Lost | VDD Off | Backup Reg ON\n~2-3 µA", (8, 5.0), "#FECDD3", COLOR_ALERT_RED)
    ]
    for text, (mx, my), bg, border in modes:
        rect = patches.FancyBboxPatch((mx-1.8, my-0.9), 3.6, 1.8, boxstyle="round,pad=0.1", facecolor=bg, edgecolor=border, lw=1.8)
        ax1.add_patch(rect)
        ax1.text(mx, my, text, fontsize=8, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")
        
    # Transition labels
    ax1.annotate("WFI / WFE", xy=(2.5, 6.0), xytext=(4.0, 7.8), arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_NAVY_PRIMARY))
    ax1.annotate("Any Interrupt", xy=(4.5, 8.0), xytext=(2.8, 6.5), arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_GREEN))
    
    ax1.annotate("PWR_EnterSTOPMode()", xy=(5.0, 2.8), xytext=(5.0, 7.5), arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_ALERT_ORANGE))
    ax1.annotate("EXTI / RTC Alarm\n(Must restore clock!)", xy=(5.8, 7.5), xytext=(5.8, 3.0), arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_BLUE_SECONDARY))

    # Current Timeline
    ax2.set_facecolor("white")
    ax2.set_title("(B) Periodic Duty-Cycling Profile: Active Burst vs. Deep Sleep", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    t = [0, 10, 10, 12, 12, 100, 100, 102, 102, 190]
    curr = [0.02, 0.02, 35, 35, 0.02, 0.02, 35, 35, 0.02, 0.02]
    ax2.plot(t, curr, color=COLOR_BLUE_SECONDARY, lw=2)
    ax2.fill_between(t, curr, color="#BAE6FD", alpha=0.4)
    ax2.set_xlabel("Time (milliseconds)", fontsize=9, fontweight="bold")
    ax2.set_ylabel("Current Consumption (mA)", fontsize=9, fontweight="bold")
    ax2.set_ylim(-1, 40)
    ax2.grid(True, linestyle="--", alpha=0.5)
    
    ax2.annotate("Active Burst (2 ms, 35 mA)\nRead Sensor + TX Telemetry", xy=(11, 35), xytext=(25, 32),
                 arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_ALERT_RED), fontsize=8, fontweight="bold")
    ax2.annotate("Stop Mode (98 ms, 20 µA)\nSRAM retained, waiting for RTC", xy=(50, 0.5), xytext=(40, 15),
                 arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_GREEN), fontsize=8, fontweight="bold")
    
    ax2.text(100, -8, "Average Current: I_avg = (2 ms * 35 mA + 98 ms * 0.02 mA) / 100 ms ≈ 0.72 mA\nBattery Life on 1000 mAh LiPo: 1000 mAh / 0.72 mA ≈ 1,388 hours (57.8 days)!",
             ha="center", fontsize=8.5, bbox=dict(boxstyle="round,pad=0.4", facecolor="#FEF3C7", edgecolor=COLOR_ALERT_ORANGE))
    
    plt.tight_layout()
    save_fig(fig, "fig3_low_power_modes.png")

# =========================================================================
# FIGURE 4: Build Pipeline & Memory Layout
# =========================================================================
def make_fig4():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6.5))
    fig.patch.set_facecolor("white")
    
    # Subplot A: Build pipeline
    ax1.set_facecolor(COLOR_LIGHT_BG)
    ax1.set_xlim(0, 10)
    ax1.set_ylim(0, 10)
    ax1.axis("off")
    ax1.set_title("(A) Compilation, Linking, and Startup Sequence", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    steps = [
        ("Source Code (*.c, *.h)", 9.0, "#E2E8F0"),
        ("Preprocessor (arm-none-eabi-gcc -E) -> *.i", 7.8, "#E2E8F0"),
        ("Compiler (arm-none-eabi-gcc -S) -> *.s", 6.6, "#E2E8F0"),
        ("Assembler (arm-none-eabi-as) -> *.o Objects", 5.4, "#BAE6FD"),
        ("Linker (arm-none-eabi-ld + *.ld script) -> *.elf", 4.0, "#7DD3FC"),
        ("Binary Generator (objcopy) -> *.bin / *.hex", 2.6, "#BBF7D0"),
        ("ST-LINK Programmer -> STM32 On-Chip Flash", 1.2, "#FED7AA")
    ]
    for text, y, col in steps:
        rect = patches.FancyBboxPatch((0.8, y-0.4), 8.4, 0.8, boxstyle="round,pad=0.05", facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, lw=1.2)
        ax1.add_patch(rect)
        ax1.text(5.0, y, text, fontsize=8.5, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")
        if y > 1.5:
            ax1.annotate("", xy=(5.0, y-0.4), xytext=(5.0, y-0.7), arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_NAVY_PRIMARY))

    # Subplot B: Memory Layout Diagram (Flash vs SRAM)
    ax2.set_facecolor(COLOR_LIGHT_BG)
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 10)
    ax2.axis("off")
    ax2.set_title("(B) Physical Memory Layout (Flash vs. SRAM)", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    # Flash Block
    ax2.text(2.5, 9.4, "FLASH MEMORY (0x0800 0000)", fontsize=9.5, fontweight="bold", ha="center", color=COLOR_BLUE_SECONDARY)
    flash_sects = [
        (".isr_vector (Interrupt Vector Table)", 8.5, 0.7, "#E2E8F0"),
        (".text (Compiled Program Instructions)", 6.8, 1.4, "#BAE6FD"),
        (".rodata (Constants, Strings)", 5.6, 0.8, "#CBD5E1"),
        (".data initializers (Copy to RAM at reset)", 4.6, 0.8, "#FED7AA")
    ]
    for name, y, h, col in flash_sects:
        rect = patches.Rectangle((0.6, y), 3.8, h, facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, lw=1)
        ax2.add_patch(rect)
        ax2.text(2.5, y + h/2, name, fontsize=7.5, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")
        
    # SRAM Block
    ax2.text(7.5, 9.4, "SRAM (0x2000 0000)", fontsize=9.5, fontweight="bold", ha="center", color=COLOR_GREEN)
    sram_sects = [
        (".data (Initialized global/static vars)", 8.2, 1.0, "#FED7AA"),
        (".bss (Zero-initialized vars)", 6.8, 1.1, "#BBF7D0"),
        ("Heap (malloc, grows downwards ->)", 5.2, 1.2, "#FEF3C7"),
        ("<- Free Stack/Heap Space ->", 3.8, 1.1, "white"),
        ("Stack (Local vars, grows upwards <-)", 2.4, 1.1, "#FEE2E2"),
        ("Top of Stack (_estack: 0x2000 5000)", 1.4, 0.7, "#E2E8F0")
    ]
    for name, y, h, col in sram_sects:
        rect = patches.Rectangle((5.6, y), 3.8, h, facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, lw=1)
        ax2.add_patch(rect)
        ax2.text(7.5, y + h/2, name, fontsize=7.5, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")

    # Reset Handler copy arrow
    ax2.annotate("Reset_Handler copies\n.data from Flash to RAM!", xy=(5.6, 8.7), xytext=(4.0, 5.0),
                 arrowprops=dict(facecolor=COLOR_ALERT_ORANGE, shrink=0.08, width=1.5, headwidth=6),
                 fontsize=8, fontweight="bold", color=COLOR_NAVY_PRIMARY)

    plt.tight_layout()
    save_fig(fig, "fig4_build_and_memory_layout.png")

# =========================================================================
# FIGURE 5: Actuator Interfacing & Power Electronics
# =========================================================================
def make_fig5():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.patch.set_facecolor("white")
    
    # Subplot A: H-Bridge Circuit Schematic
    ax1.set_facecolor(COLOR_LIGHT_BG)
    ax1.set_xlim(0, 10)
    ax1.set_ylim(0, 10)
    ax1.axis("off")
    ax1.set_title("(A) Full-Bridge (H-Bridge) DC Motor Drive & Flyback Protection", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    # Vmotor and GND
    ax1.plot([2, 8], [9, 9], color=COLOR_ALERT_RED, lw=3)
    ax1.text(5, 9.3, "+VMOTOR (6V - 24V)", fontsize=10, fontweight="bold", ha="center", color=COLOR_ALERT_RED)
    ax1.plot([2, 8], [1, 1], color=COLOR_NAVY_PRIMARY, lw=3)
    ax1.text(5, 0.5, "POWER GND", fontsize=10, fontweight="bold", ha="center", color=COLOR_NAVY_PRIMARY)
    
    # Switches
    switches = [("Q1 (High-Side)", (3, 7.5)), ("Q2 (High-Side)", (7, 7.5)),
                ("Q3 (Low-Side)",  (3, 2.5)), ("Q4 (Low-Side)",  (7, 2.5))]
    for name, (sx, sy) in switches:
        rect = patches.FancyBboxPatch((sx-0.9, sy-0.6), 1.8, 1.2, boxstyle="round,pad=0.05", facecolor="#E2E8F0", edgecolor=COLOR_NAVY_PRIMARY, lw=1.2)
        ax1.add_patch(rect)
        ax1.text(sx, sy, name, fontsize=7.5, ha="center", va="center", fontweight="bold")
        # Flyback diode symbol indicator
        ax1.text(sx + 0.6, sy + 0.7, "[D]", fontsize=7, color=COLOR_ALERT_ORANGE, fontweight="bold")

    # Motor in center
    m_circle = patches.Circle((5, 5), 1.0, facecolor="#BAE6FD", edgecolor=COLOR_NAVY_PRIMARY, lw=2)
    ax1.add_patch(m_circle)
    ax1.text(5, 5, "MOTOR\n[ M ]", fontsize=9, fontweight="bold", ha="center", va="center", color=COLOR_NAVY_PRIMARY)
    
    # Connecting wires
    ax1.plot([3, 3], [9, 8.1], color="black", lw=1.5)
    ax1.plot([7, 7], [9, 8.1], color="black", lw=1.5)
    ax1.plot([3, 3], [6.9, 5], color="black", lw=1.5)
    ax1.plot([3, 4], [5, 5], color="black", lw=1.5)
    ax1.plot([7, 7], [6.9, 5], color="black", lw=1.5)
    ax1.plot([6, 7], [5, 5], color="black", lw=1.5)
    ax1.plot([3, 3], [5, 3.1], color="black", lw=1.5)
    ax1.plot([7, 7], [5, 3.1], color="black", lw=1.5)
    ax1.plot([3, 3], [1.9, 1], color="black", lw=1.5)
    ax1.plot([7, 7], [1.9, 1], color="black", lw=1.5)
    
    # States legend
    ax1.text(5, 3.2, "Forward: Q1 + Q4 ON\nReverse: Q2 + Q3 ON\nBrake: Q3 + Q4 ON (Shorts back-EMF)\nShoot-Through: Q1+Q3 ON (Destroys FETs!)", 
             fontsize=8, ha="center", bbox=dict(boxstyle="round,pad=0.3", facecolor="#FEF3C7", edgecolor=COLOR_ALERT_ORANGE))

    # Subplot B: Servo & Stepper Signals
    ax2.set_facecolor(COLOR_LIGHT_BG)
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 10)
    ax2.axis("off")
    ax2.set_title("(B) Servo 50 Hz PWM & Stepper Step/Dir Control", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    # Servo Waveforms
    ax2.text(0.5, 9.2, "1. Hobby Servo Control (50 Hz / 20 ms Period)", fontsize=9.5, fontweight="bold", color=COLOR_BLUE_SECONDARY)
    # 1.0 ms pulse (0 deg)
    ax2.plot([0.5, 1.0, 1.0, 2.0, 2.0, 9.5], [7.8, 7.8, 8.6, 8.6, 7.8, 7.8], color=COLOR_NAVY_PRIMARY, lw=1.8)
    ax2.text(1.5, 8.8, "1.0 ms pulse = 0°", fontsize=7.5, color=COLOR_NAVY_PRIMARY, fontweight="bold")
    # 1.5 ms pulse (90 deg)
    ax2.plot([0.5, 1.0, 1.0, 2.5, 2.5, 9.5], [6.4, 6.4, 7.2, 7.2, 6.4, 6.4], color=COLOR_TEAL_ACCENT, lw=1.8)
    ax2.text(1.75, 7.4, "1.5 ms pulse = 90° (Neutral)", fontsize=7.5, color=COLOR_TEAL_ACCENT, fontweight="bold")
    # 2.0 ms pulse (180 deg)
    ax2.plot([0.5, 1.0, 1.0, 3.0, 3.0, 9.5], [5.0, 5.0, 5.8, 5.8, 5.0, 5.0], color=COLOR_ALERT_ORANGE, lw=1.8)
    ax2.text(2.0, 6.0, "2.0 ms pulse = 180°", fontsize=7.5, color=COLOR_ALERT_ORANGE, fontweight="bold")
    
    # Stepper Waveforms
    ax2.text(0.5, 4.0, "2. Stepper Driver Interface (A4988 / DRV8825)", fontsize=9.5, fontweight="bold", color=COLOR_BLUE_SECONDARY)
    # DIR Pin
    ax2.text(0.5, 3.2, "DIR Pin:", fontsize=8, fontweight="bold")
    ax2.plot([2.0, 5.5, 5.5, 9.5], [3.2, 3.2, 2.4, 2.4], color=COLOR_GREEN, lw=2)
    ax2.text(3.5, 3.5, "HIGH (CW)", fontsize=7.5, color=COLOR_GREEN, fontweight="bold")
    ax2.text(7.5, 2.7, "LOW (CCW)", fontsize=7.5, color=COLOR_GREEN, fontweight="bold")
    # STEP Pin
    ax2.text(0.5, 1.5, "STEP Pin:", fontsize=8, fontweight="bold")
    step_x = [2.0, 2.3, 2.3, 2.6, 2.6, 3.2, 3.2, 3.5, 3.5, 4.0, 4.0, 4.3, 4.3, 6.0, 6.0, 6.3, 6.3, 7.0, 7.0, 7.3, 7.3, 9.5]
    step_y = [1.2, 1.2, 2.0, 2.0, 1.2, 1.2, 2.0, 2.0, 1.2, 1.2, 2.0, 2.0, 1.2, 1.2, 2.0, 2.0, 1.2, 1.2, 2.0, 2.0, 1.2, 1.2]
    ax2.plot(step_x, step_y, color=COLOR_NAVY_PRIMARY, lw=1.8)
    ax2.text(5.5, 0.5, "Each pulse = 1 step (or microstep 1/16)\nFrequency determines rotational speed (RPM)", fontsize=8, ha="center", style="italic")

    plt.tight_layout()
    save_fig(fig, "fig5_actuators_and_power.png")

# =========================================================================
# FIGURE 6: Watchdog Timers: IWDG vs WWDG & FreeRTOS Supervision
# =========================================================================
def make_fig6():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.patch.set_facecolor("white")
    
    # Subplot A: Timing Windows of IWDG vs WWDG
    ax1.set_facecolor(COLOR_LIGHT_BG)
    ax1.set_xlim(0, 10)
    ax1.set_ylim(0, 10)
    ax1.axis("off")
    ax1.set_title("(A) Watchdog Timing Windows: IWDG vs. WWDG", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    # IWDG Bar
    ax1.text(0.5, 8.8, "1. Independent Watchdog (IWDG): Clocked by 40 kHz LSI", fontsize=9, fontweight="bold", color=COLOR_BLUE_SECONDARY)
    rect_iwdg = patches.Rectangle((0.5, 7.2), 9.0, 1.2, facecolor="#BBF7D0", edgecolor=COLOR_GREEN, lw=1.5)
    ax1.add_patch(rect_iwdg)
    ax1.text(5.0, 7.8, "SAFE REFRESH WINDOW: Refresh allowed at ANY time before timeout", fontsize=8.5, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")
    ax1.axvline(9.5, 0.65, 0.85, color=COLOR_ALERT_RED, lw=3)
    ax1.text(9.5, 6.7, "TIMEOUT -> MCU RESET", fontsize=8, color=COLOR_ALERT_RED, fontweight="bold", ha="right")

    # WWDG Bar
    ax1.text(0.5, 5.5, "2. Window Watchdog (WWDG): Clocked by APB1 Bus", fontsize=9, fontweight="bold", color=COLOR_BLUE_SECONDARY)
    # Forbidden early refresh zone
    rect_early = patches.Rectangle((0.5, 3.8), 3.5, 1.2, facecolor="#FECDD3", edgecolor=COLOR_ALERT_RED, lw=1.5)
    ax1.add_patch(rect_early)
    ax1.text(2.25, 4.4, "FORBIDDEN EARLY ZONE\nRefresh here -> RESET!", fontsize=8, ha="center", va="center", color=COLOR_ALERT_RED, fontweight="bold")
    # Allowed window
    rect_allowed = patches.Rectangle((4.0, 3.8), 4.0, 1.2, facecolor="#BBF7D0", edgecolor=COLOR_GREEN, lw=1.5)
    ax1.add_patch(rect_allowed)
    ax1.text(6.0, 4.4, "PERMITTED WINDOW\n(Refresh between W & 0x40)", fontsize=8, ha="center", va="center", color=COLOR_GREEN, fontweight="bold")
    # Late zone
    rect_late = patches.Rectangle((8.0, 3.8), 1.5, 1.2, facecolor="#FED7AA", edgecolor=COLOR_ALERT_ORANGE, lw=1.5)
    ax1.add_patch(rect_late)
    ax1.text(8.75, 4.4, "TIMEOUT\nRESET", fontsize=8, ha="center", va="center", color=COLOR_ALERT_ORANGE, fontweight="bold")

    ax1.text(5.0, 2.5, "WWDG detects runaway code that refreshes TOO FAST (e.g. trapped in a fast loop)\nas well as firmware that hangs and fails to refresh!", 
             fontsize=8.5, ha="center", bbox=dict(boxstyle="round,pad=0.4", facecolor="white", edgecolor=COLOR_TEAL_ACCENT))

    # Subplot B: FreeRTOS Multi-Task Watchdog Supervisor
    ax2.set_facecolor(COLOR_LIGHT_BG)
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 10)
    ax2.axis("off")
    ax2.set_title("(B) FreeRTOS Multi-Task Watchdog Supervisor Pattern", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    tasks = [
        ("Task 1: Sensor Task (50 Hz)\nSets Event Bit 0x01", (1.0, 7.5)),
        ("Task 2: Motor PID (200 Hz)\nSets Event Bit 0x02", (1.0, 5.0)),
        ("Task 3: Telemetry (10 Hz)\nSets Event Bit 0x04", (1.0, 2.5)),
    ]
    for text, (tx, ty) in tasks:
        rect = patches.FancyBboxPatch((tx, ty), 3.5, 1.4, boxstyle="round,pad=0.08", facecolor="#BAE6FD", edgecolor=COLOR_NAVY_PRIMARY, lw=1.2)
        ax2.add_patch(rect)
        ax2.text(tx + 1.75, ty + 0.7, text, fontsize=7.5, ha="center", va="center", fontweight="bold")
        # Arrow to Event Group
        ax2.annotate("", xy=(5.2, 5.7), xytext=(4.5, ty + 0.7), arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_BLUE_SECONDARY))

    # Event Group Box
    rect_eg = patches.FancyBboxPatch((5.2, 4.5), 1.8, 2.4, boxstyle="round,pad=0.08", facecolor="#FEF3C7", edgecolor=COLOR_ALERT_ORANGE, lw=1.5)
    ax2.add_patch(rect_eg)
    ax2.text(6.1, 5.7, "FreeRTOS\nEvent Group\n[0x07]", fontsize=8, ha="center", va="center", fontweight="bold", color=COLOR_ALERT_ORANGE)
    
    # Watchdog Task Box
    rect_wt = patches.FancyBboxPatch((7.6, 4.5), 2.0, 2.4, boxstyle="round,pad=0.08", facecolor="#BBF7D0", edgecolor=COLOR_GREEN, lw=1.5)
    ax2.add_patch(rect_wt)
    ax2.text(8.6, 5.7, "Watchdog\nDaemon Task\nWaits for 0x07\nRefreshes IWDG", fontsize=7.5, ha="center", va="center", fontweight="bold", color=COLOR_GREEN)
    
    ax2.annotate("", xy=(7.6, 5.7), xytext=(7.0, 5.7), arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_NAVY_PRIMARY))
    
    ax2.text(5.0, 1.2, "GOLDEN RULE: Never refresh the watchdog inside a Timer ISR!\nIf any individual task hangs or deadlocks, the bit is NOT set,\nand the Watchdog Task lets the hardware reset the MCU.",
             fontsize=8, ha="center", bbox=dict(boxstyle="round,pad=0.4", facecolor="#FEE2E2", edgecolor=COLOR_ALERT_RED))

    plt.tight_layout()
    save_fig(fig, "fig6_watchdog_architectures.png")

# =========================================================================
# FIGURE 7: Bootloader Partitioning & Vector Table Offset (VTOR)
# =========================================================================
def make_fig7():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6.5))
    fig.patch.set_facecolor("white")
    
    # Subplot A: Flash Memory Partitioning
    ax1.set_facecolor(COLOR_LIGHT_BG)
    ax1.set_xlim(0, 10)
    ax1.set_ylim(0, 10)
    ax1.axis("off")
    ax1.set_title("(A) On-Chip Flash Memory Partitioning", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    partitions = [
        ("BOOTLOADER REGION (16 KB)\n0x0800 0000 - 0x0800 3FFF\n• Check Application Integrity (CRC32)\n• Listen on UART/USB for updates\n• Jump to Application", 7.0, 2.2, "#BAE6FD", COLOR_BLUE_SECONDARY),
        ("METADATA HEADER (1 KB Page)\n0x0800 4000 - 0x0800 43FF\n• Magic Word: 0x55AA55AA\n• Version: v2.1.0\n• Size: 48,256 Bytes | CRC32", 4.4, 2.0, "#FEF3C7", COLOR_ALERT_ORANGE),
        ("USER APPLICATION REGION (47 KB)\n0x0800 4400 - 0x0800 FFFF\n• Application Vector Table at 0x0800 4400\n• Main Firmware Logic (Robotics Control)", 1.6, 2.4, "#BBF7D0", COLOR_GREEN),
    ]
    for text, y, h, bg, border in partitions:
        rect = patches.FancyBboxPatch((0.8, y), 8.4, h, boxstyle="round,pad=0.08", facecolor=bg, edgecolor=border, lw=1.8)
        ax1.add_patch(rect)
        ax1.text(5.0, y + h/2, text, fontsize=8, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")

    # Subplot B: Jump to Application Flowchart
    ax2.set_facecolor(COLOR_LIGHT_BG)
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 10)
    ax2.axis("off")
    ax2.set_title("(B) Application Jump & Vector Relocation Execution Flow", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    flow_steps = [
        ("1. Validate Application Stack Pointer\nCheck (*(uint32_t*)APP_ADDR & 0x2FFE0000) == 0x20000000", 8.8, "#E2E8F0"),
        ("2. Disable Global Interrupts\n__disable_irq() and disable SysTick", 7.4, "#E2E8F0"),
        ("3. Deinitialize Peripherals & HAL\nHAL_DeInit() and clear pending IRQs", 6.0, "#E2E8F0"),
        ("4. Relocate Vector Table in NVIC\nSCB->VTOR = APPLICATION_ADDRESS", 4.6, "#BAE6FD"),
        ("5. Set Main Stack Pointer (MSP)\n__set_MSP(*(volatile uint32_t*)APP_ADDR)", 3.2, "#7DD3FC"),
        ("6. Fetch Reset_Handler & Jump!\nvoid (*app_reset)(void) = *(APP_ADDR + 4);\napp_reset();", 1.4, "#BBF7D0")
    ]
    for text, y, col in flow_steps:
        rect = patches.FancyBboxPatch((0.8, y-0.45), 8.4, 0.9, boxstyle="round,pad=0.05", facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, lw=1.2)
        ax2.add_patch(rect)
        ax2.text(5.0, y, text, fontsize=7.5, ha="center", va="center", color=COLOR_TEXT_DARK, fontweight="bold")
        if y > 1.8:
            ax2.annotate("", xy=(5.0, y-0.45), xytext=(5.0, y-0.8), arrowprops=dict(arrowstyle="->", lw=1.5, color=COLOR_NAVY_PRIMARY))

    plt.tight_layout()
    save_fig(fig, "fig7_bootloader_and_vtor.png")

# =========================================================================
# FIGURE 8: DAC Waveform Generation Pipeline vs PWM Filter
# =========================================================================
def make_fig8():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.patch.set_facecolor("white")
    
    # Subplot A: Hardware Timer -> DMA -> DAC
    ax1.set_facecolor(COLOR_LIGHT_BG)
    ax1.set_xlim(0, 10)
    ax1.set_ylim(0, 10)
    ax1.axis("off")
    ax1.set_title("(A) Hardware True DAC Waveform Generation (STM32F405)", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    dac_blocks = [
        ("TIM6 Timer (TRGO Event)\nPeriodic Tick: F_tim = N * F_wave", (0.5, 7.0), (3.8, 1.8), "#BAE6FD"),
        ("DMA1 Stream 5\nCircular Transfer: LUT -> DAC", (5.7, 7.0), (3.8, 1.8), "#7DD3FC"),
        ("Sine LUT in SRAM (e.g. 128 words)\n[2048, 2148, 2248, ...]", (0.5, 3.8), (3.8, 1.8), "#CBD5E1"),
        ("12-Bit DAC1 Peripheral (PA4)\nAnalog Output Buffer (0 to 3.3V)", (5.7, 3.8), (3.8, 1.8), "#BBF7D0"),
    ]
    for text, (bx, by), (bw, bh), col in dac_blocks:
        rect = patches.FancyBboxPatch((bx, by), bw, bh, boxstyle="round,pad=0.08", facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, lw=1.2)
        ax1.add_patch(rect)
        ax1.text(bx + bw/2, by + bh/2, text, fontsize=8, ha="center", va="center", fontweight="bold")
        
    ax1.annotate("Hardware TRGO Trigger", xy=(5.7, 7.9), xytext=(4.3, 7.9), arrowprops=dict(arrowstyle="->", lw=1.8, color=COLOR_BLUE_SECONDARY))
    ax1.annotate("DMA Reads SRAM", xy=(4.3, 4.7), xytext=(5.7, 7.0), arrowprops=dict(arrowstyle="->", lw=1.8, color=COLOR_NAVY_PRIMARY))
    ax1.annotate("DMA Writes DAC_DHR12R1", xy=(7.6, 5.6), xytext=(7.6, 7.0), arrowprops=dict(arrowstyle="->", lw=1.8, color=COLOR_GREEN))

    # Sine wave visualization
    t = np.linspace(0, 2*np.pi, 100)
    sine = 1.65 + 1.2 * np.sin(t)
    ax1_sub = ax1.inset_axes([0.15, 0.08, 0.7, 0.22])
    ax1_sub.plot(t, sine, color=COLOR_NAVY_PRIMARY, lw=2)
    ax1_sub.set_title("Analog Output Pin PA4 (Continuous Smooth Waveform, 0% CPU Load)", fontsize=8, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    ax1_sub.set_ylim(0, 3.3)
    ax1_sub.grid(True, linestyle="--", alpha=0.5)

    # Subplot B: STM32F103 PWM + RC Filter Alternative
    ax2.set_facecolor(COLOR_LIGHT_BG)
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 10)
    ax2.axis("off")
    ax2.set_title("(B) STM32F103 Alternative: High-Speed PWM + 2nd-Order RC Filter", fontsize=11, fontweight="bold", color=COLOR_NAVY_PRIMARY)
    
    pwm_blocks = [
        ("TIM1 / TIM2 PWM Channel\n(High Frequency: e.g. 100 kHz)\nVariable Duty Cycle ~ Output Voltage", (0.5, 6.5), (4.0, 2.2), "#FED7AA"),
        ("2nd-Order Analog RC Filter\nLow-pass cutoff fc << PWM Freq\nAttenuates 100 kHz carrier", (5.5, 6.5), (4.0, 2.2), "#FEF3C7"),
    ]
    for text, (bx, by), (bw, bh), col in pwm_blocks:
        rect = patches.FancyBboxPatch((bx, by), bw, bh, boxstyle="round,pad=0.08", facecolor=col, edgecolor=COLOR_NAVY_PRIMARY, lw=1.2)
        ax2.add_patch(rect)
        ax2.text(bx + bw/2, by + bh/2, text, fontsize=8, ha="center", va="center", fontweight="bold")
        
    ax2.annotate("High-Speed PWM", xy=(5.5, 7.6), xytext=(4.5, 7.6), arrowprops=dict(arrowstyle="->", lw=1.8, color=COLOR_ALERT_ORANGE))
    
    # Filter schematic sketch
    ax2.text(5.0, 4.8, "PWM Out (PA8) ───[ 10 kΩ ]───┬───[ 10 kΩ ]───┬───► Analog Out\n                             │               │\n                          [ 100 nF ]      [ 100 nF ]\n                             │               │\n                            GND             GND", 
             fontsize=8, family="monospace", ha="center", bbox=dict(boxstyle="square,pad=0.4", facecolor="white", edgecolor=COLOR_BORDER_GRAY))

    ax2.text(5.0, 1.5, "Trade-off on STM32F103 without internal DAC:\nFilter introduces settling delay (phase lag) and residual ripple voltage.\nFor high-fidelity audio or high-speed arbitrary waveforms, add an external I²C/SPI DAC (e.g. MCP4725)!", 
             fontsize=8, ha="center", bbox=dict(boxstyle="round,pad=0.4", facecolor="#FEF3C7", edgecolor=COLOR_ALERT_ORANGE))

    plt.tight_layout()
    save_fig(fig, "fig8_dac_and_waveform.png")

if __name__ == "__main__":
    print("Generating 8 technical architecture figures for Advanced Topics Guide...")
    make_fig1()
    make_fig2()
    make_fig3()
    make_fig4()
    make_fig5()
    make_fig6()
    make_fig7()
    make_fig8()
    print("All 8 figures successfully generated in diagrams_adv/")
