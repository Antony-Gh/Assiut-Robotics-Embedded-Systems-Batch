"""Generate all figures for the STM32 reference (fig1-fig6 reuse the verified base
generator with technical corrections; fig7-fig8 are new)."""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(HERE)

src = open(os.path.join(HERE, "make_figures_base.py"), encoding="utf-8").read()
src = src.replace("SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))", f'SCRIPT_DIR = r"{BASE}"')
exec(compile(src, "make_figures_base.py", "exec"), globals())

import numpy as np  # noqa: E402

# ------------------------------------------------------------------------------
# FIGURE 7: Interrupt execution flow + DMA transfer architecture
# ------------------------------------------------------------------------------
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13.5, 6.6), dpi=220)
fig.patch.set_facecolor("white")

ax1.set_xlim(0, 10)
ax1.set_ylim(0, 12)
ax1.axis("off")
ax1.set_title("A. Interrupt Execution Flow (Cortex-M + NVIC + HAL)", fontsize=10.5, fontweight="bold", color=NAVY)

flow = [
    ("1. Peripheral event", "Flag set in status register (e.g. TIM UIF, USART RXNE)", ICE, BLUE),
    ("2. NVIC arbitration", "Enabled? Pending? Priority higher than current context?", MINT, TEAL),
    ("3. Hardware stacking", "CPU pushes R0-R3, R12, LR, PC, xPSR (~12 cycles)", AMBER, ORANGE),
    ("4. Vector fetch", "PC = vector_table[IRQn + 16] -> e.g. TIM2_IRQHandler", "#EDE9FE", "#5B21B6"),
    ("5. Handler (it.c)", "TIM2_IRQHandler() -> HAL_TIM_IRQHandler(&htim2)", ICE, BLUE),
    ("6. HAL callback", "Flag cleared by HAL; HAL_TIM_PeriodElapsedCallback() (ISR context)", MINT, TEAL),
    ("7. Exception return", "Unstack registers; main loop / task resumes where it stopped", "#DCFCE7", "#166534"),
]
y = 10.3
for title, sub, bg, edge in flow:
    draw_box(ax1, 0.5, y, 9.0, 1.15, title, bg=bg, edge=edge, fontsize=8.8, subtext=sub, subsize=7.2)
    y -= 1.5
for i in range(len(flow) - 1):
    ytop = 10.3 - i * 1.5
    draw_arrow(ax1, 5.0, ytop, 5.0, ytop - 0.35, color=NAVY, scale=11)

ax2.set_xlim(0, 10)
ax2.set_ylim(0, 12)
ax2.axis("off")
ax2.set_title("B. DMA Transfer Architecture (Peripheral -> Memory)", fontsize=10.5, fontweight="bold", color=NAVY)

draw_box(ax2, 0.4, 9.6, 4.2, 1.6, "CPU (Bus Master 1)", bg="#DBEAFE", edge=NAVY, fontsize=9,
         subtext="Configures DMA once;\nhandles HT / TC / TE events", subsize=7.4)
draw_box(ax2, 5.4, 9.6, 4.2, 1.6, "DMA Controller (Bus Master 2)", bg=AMBER, edge=ORANGE, fontsize=9,
         subtext="CNDTR / NDTR, CPAR / PAR,\nCMAR / M0AR, CCR / CR", subsize=7.4)
draw_box(ax2, 0.4, 6.6, 9.2, 1.5, "AHB Bus Matrix / Arbiter", bg="#F1F5F9", edge=SLATE, fontsize=9,
         subtext="Round-robin or priority arbitration between CPU and DMA accesses", subsize=7.4)
draw_arrow(ax2, 2.5, 9.6, 2.5, 8.1, color=NAVY, style="<->")
draw_arrow(ax2, 7.5, 9.6, 7.5, 8.1, color=ORANGE, style="<->")
draw_box(ax2, 0.4, 3.6, 4.2, 1.8, "Peripheral Data Register", bg=MINT, edge=TEAL, fontsize=9,
         subtext="e.g. ADC1->DR, USART2->DR\n(address fixed: no increment)", subsize=7.4)
draw_box(ax2, 5.4, 3.6, 4.2, 1.8, "SRAM Buffer", bg="#EDE9FE", edge="#5B21B6", fontsize=9,
         subtext="uint16_t adc_buf[N]\n(address increments per item)", subsize=7.4)
draw_arrow(ax2, 2.5, 6.6, 2.5, 5.4, color=TEAL, style="<->")
draw_arrow(ax2, 7.5, 6.6, 7.5, 5.4, color="#5B21B6", style="<->")
draw_box(ax2, 0.4, 0.5, 9.2, 2.1, "Transfer cycle (per request)", bg="#F8FAFC", edge=SLATE, fontsize=9,
         subtext="1) Peripheral asserts DMA request (e.g. ADC EOC)  2) DMA reads peripheral register\n"
                 "3) DMA writes SRAM[idx++]  4) remaining count--  5) at N/2: HT flag, at N: TC flag (IRQ)\n"
                 "6) Normal mode stops; circular mode reloads count and address and continues", subsize=7.2)

plt.tight_layout()
plt.savefig(os.path.join(DIAGRAMS_DIR, "fig7_interrupt_dma_architecture.png"), dpi=220)
plt.close()

# ------------------------------------------------------------------------------
# FIGURE 8: ADC conversion chain + quantization staircase
# ------------------------------------------------------------------------------
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13.5, 5.6), dpi=220, gridspec_kw={"width_ratios": [1.15, 1]})
fig.patch.set_facecolor("white")

ax1.set_xlim(0, 10)
ax1.set_ylim(0, 10)
ax1.axis("off")
ax1.set_title("A. SAR ADC Conversion Chain", fontsize=10.5, fontweight="bold", color=NAVY)
chain = [
    ("Sensor / divider", "Source resistance R_source", "#F1F5F9", SLATE),
    ("Anti-alias RC filter", "Corner frequency below F_s / 2", ICE, BLUE),
    ("GPIO analog mode + channel mux", "Schmitt trigger off, analog switch on", MINT, TEAL),
    ("Sample & hold capacitor", "R_source * C_in must settle within T_sample", AMBER, ORANGE),
    ("SAR binary search", "N comparisons against V_REF DAC (N = 12)", "#EDE9FE", "#5B21B6"),
    ("ADC data register", "Right/left aligned code 0 .. 2^N - 1", "#DCFCE7", "#166534"),
]
yy = 8.5
for title, sub, bg, edge in chain:
    draw_box(ax1, 0.8, yy, 8.4, 1.15, title, bg=bg, edge=edge, fontsize=8.6, subtext=sub, subsize=7.2)
    yy -= 1.55
for i in range(len(chain) - 1):
    ytop = 8.5 - i * 1.55
    draw_arrow(ax1, 5.0, ytop, 5.0, ytop - 0.4, color=NAVY, scale=11)

# Quantization staircase for an illustrative 3-bit converter
vref = 3.3
n_bits = 3
levels = 2 ** n_bits
v = np.linspace(0, vref, 600)
code_ideal = np.clip(np.floor(v / vref * (levels)), 0, levels - 1)
ax2.step(v, code_ideal, where="post", color=TEAL, linewidth=2.0, label="Quantized code (3-bit illustration)")
ax2.plot(v, v / vref * levels, color=NAVY, linestyle="--", linewidth=1.2, label="Ideal analog transfer")
ax2.set_xlim(0, vref)
ax2.set_ylim(0, levels)
ax2.set_xlabel("Input voltage V_in (V)", fontsize=9, color=NAVY)
ax2.set_ylabel("Output code", fontsize=9, color=NAVY)
ax2.set_title("B. Quantization: 1 LSB = V_REF / 2^N (illustration)", fontsize=10.5, fontweight="bold", color=NAVY)
ax2.set_yticks(range(levels))
ax2.grid(True, color="#CBD5E1", linewidth=0.6)
ax2.legend(loc="upper left", fontsize=8, frameon=True)
ax2.annotate("Quantization error: 0..1 LSB\n(floor-type staircase shown;\n+/-0.5 LSB if centered)", xy=(1.55, 3.0),
             xytext=(1.75, 0.5), fontsize=8, color=ORANGE, arrowprops=dict(arrowstyle="->", color=ORANGE))
for s in ("top", "right"):
    ax2.spines[s].set_visible(False)

plt.tight_layout()
plt.savefig(os.path.join(DIAGRAMS_DIR, "fig8_adc_chain_quantization.png"), dpi=220)
plt.close()

print("Figures 1-8 generated in", DIAGRAMS_DIR)
