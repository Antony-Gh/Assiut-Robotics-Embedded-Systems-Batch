## 3.6 Timers

### 3.6.1 Fundamentals

A **timer** is a hardware counter clocked from a prescaled clock, with compare, capture and trigger logic around it. It is not `HAL_Delay()`: once started it runs without CPU involvement and can toggle pins, request DMA, trigger an ADC or raise an interrupt at exact times.

**Table 3.5 — Timer categories**

| Category | Typical capabilities | Notes |
| --- | --- | --- |
| Basic (TIM6/TIM7) | Time base, update event, trigger output (TRGO) | Not present on every part. **STM32F103C8T6 has none** (it has TIM1–TIM4) |
| General purpose (TIM2–TIM5, …) | Time base, input capture, output compare, PWM, encoder interface, one-pulse | 16-bit; TIM2 and TIM5 are 32-bit on F4 |
| Advanced control (TIM1, TIM8) | Complementary PWM, dead-time insertion, break input, repetition counter | Motor-control oriented |
| Low-power timer | Timing in stop/low-power modes | Selected devices |
| SysTick | Core timer; HAL tick and RTOS tick | Not a TIM peripheral |
| IWDG / WWDG | Watchdogs | Safety timeouts, not general timing (Section 3.6.9) |

**The core registers**

- **PSC (prescaler):** divides the timer kernel clock by `PSC + 1`; 16-bit.
- **CNT (counter):** the live count (up, down or center-aligned).
- **ARR (auto-reload):** the period; when CNT reaches ARR the counter wraps and an **update event (UEV)** occurs.
- **CCR1–CCR4 (capture/compare):** compared with CNT to create PWM edges or output-compare events, or loaded with CNT on an input edge.
- **Preload (ARPE, OCxPE):** ARR/CCR writes take effect at the next update event, which prevents glitches when changing frequency or duty on the fly.

![Figure 3.3 — Timer architecture and the timer-triggered ADC/DMA pipeline](diagrams/fig4_timer_adc_dma.png)

*Figure 3.3 — (A) Timer core: kernel clock → PSC → CNT with ARR and CCR; (B) deterministic acquisition pipeline: timer TRGO → ADC → circular DMA → processing task.*

### 3.6.2 Timer mathematics

$$ f_{\text{cnt}} = \frac{f_{\text{TIM}}}{\text{PSC}+1} $$

$$ f_{\text{update}} = \frac{f_{\text{TIM}}}{(\text{PSC}+1)\,(\text{ARR}+1)} \qquad T_{\text{update}} = \frac{(\text{PSC}+1)(\text{ARR}+1)}{f_{\text{TIM}}} $$

**Variables.** *f*<sub>TIM</sub> is the timer kernel clock in Hz (after the APB ×2 rule), *PSC* and *ARR* are the register values (dimensionless), *f*<sub>cnt</sub> is the counter tick rate and *f*<sub>update</sub> the update-event (period) rate. With a repetition counter (advanced timers) divide additionally by (RCR + 1). In **center-aligned** mode the counter runs 0 → ARR → 0, so the period is 2 × ARR counter ticks (not ARR + 1).

**Worked examples (F103, f_TIM = 72 MHz)**

**Table 3.6 — Timer calculations**

| Goal | PSC | ARR | Check | Result |
| --- | --- | --- | --- | --- |
| 1 µs tick, 1 ms period (1 kHz) | 71 | 999 | 72 MHz / 72 = 1 MHz; 1 MHz / 1000 | 1 kHz update |
| 1 kHz tick from 10 kHz counter | 7199 | 9 | 72 MHz / 7200 = 10 kHz; / 10 | 1 kHz update |
| 1 Hz update | 7199 | 9999 | 10 kHz / 10 000 | 1 Hz |
| 50 Hz servo frame (20 ms) with 1 µs tick | 71 | 19999 | 1 MHz / 20 000 | 50 Hz |
| 20 kHz PWM, max resolution | 0 | 3599 | 72 MHz / 3600 | 20 kHz, 3600 steps (~11.8 bits) |

> [!WARNING]
> **16-bit prescaler overflow trap.** PSC is a 16-bit register (maximum 65 535). Trying to reach a 1 kHz counter from 72 MHz with `PSC = 71 999` writes `71 999 & 0xFFFF = 6 463` and the timer runs more than 11× too fast. Split the division between PSC and ARR: for 1 Hz choose PSC = 7 199 and ARR = 9 999. The same limit applies to ARR on 16-bit timers (max 65 535).

> [!TIP]
> **Spurious first interrupt.** HAL’s timer init generates an update event to latch the prescaler, which leaves the update flag set. Starting with `HAL_TIM_Base_Start_IT()` can therefore fire one interrupt immediately. Call `__HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);` before starting if the first early tick matters.

**Listing 3.4 — Periodic 1 ms timer interrupt**

```c
/* CubeMX: TIM4 internal clock, PSC = 7199, ARR = 9, NVIC TIM4 global interrupt enabled */
static volatile uint32_t g_ms_ticks;

void App_StartTick(void)
{
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    if (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK) { Error_Handler(); }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) { g_ms_ticks++; }    /* keep it this short */
}
```

### 3.6.3 PWM generation

In edge-aligned PWM mode 1 (up-counting, active-high) the output is active while `CNT < CCR` and inactive while `CNT ≥ CCR`:

$$ f_{\text{PWM}} = \frac{f_{\text{TIM}}}{(\text{PSC}+1)(\text{ARR}+1)} \qquad D = \frac{\text{CCR}}{\text{ARR}+1} \qquad \text{resolution} = \text{ARR}+1 \text{ steps} $$

*CCR = 0* gives 0 % and *CCR ≥ ARR + 1* gives 100 %. Polarity (CCER) decides which level is “active”. PWM requires the correct **alternate-function pin** for the timer channel, and `HAL_TIM_PWM_Start()` for that channel; starting only the time base is not enough.

**Listing 3.5 — Servo (50 Hz, 1–2 ms pulse) and 20 kHz motor PWM**

```c
/* TIM4: PSC = 71 (1 us tick), ARR = 19999 (20 ms frame), channel 1 PWM mode 1 */
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 1500);   /* 1.5 ms = centered servo */

/* TIM1: PSC = 0, ARR = 3599 -> 20 kHz; duty 25 % -> CCR = 0.25 * 3600 = 900 */
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);             /* TIM1 is advanced: also needs main output enable, handled by HAL */
__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 900);
```

PWM drives gate drivers, LEDs and servo inputs; it must never drive a motor directly from the pin. Advanced timers add complementary outputs and **dead time** so that the two transistors of a half-bridge are never on simultaneously, plus a break input for fault shutdown.

### 3.6.4 Input capture (measuring frequency and pulse width)

On a selected edge, hardware copies `CNT` into `CCRx` without CPU involvement, so edge timestamps are exact regardless of interrupt latency. Two consecutive captures give the period; capture on both edges (or the PWM-input configuration) gives pulse width.

**Listing 3.6 — Frequency measurement with input capture (1 µs tick, 16-bit counter)**

```c
/* TIM2 CH1 input capture rising edge, PSC = 71, ARR = 0xFFFF */
static volatile uint32_t g_ic_prev, g_period_us;
static volatile uint8_t  g_ic_first = 1U;

void Capture_Start(void) { HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1); }

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        uint32_t cur = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        if (g_ic_first) { g_ic_first = 0U; }
        else            { g_period_us = (cur - g_ic_prev) & 0xFFFFU; }  /* wrap-safe */
        g_ic_prev = cur;
    }
}
/* f = 1e6 / g_period_us. Valid when at most one counter wrap occurs between edges
   (period < 65.536 ms, i.e. f > 15.3 Hz). Extend with an update-interrupt overflow counter if needed. */
```

### 3.6.5 Output compare, one-pulse and encoder modes

- **Output compare** performs an action (toggle, set, clear, interrupt) when `CNT == CCRx`: a precise scheduler for “do this 350 µs from now”. Updating `CCRx = CCRx + delta` inside the compare interrupt produces a square wave of half-period `delta` ticks with no drift.
- **One-pulse mode** produces a single delayed pulse after a trigger (for example to start an ADC conversion at a quiet moment after a motor PWM edge).
- **Encoder mode** counts quadrature signals from two input channels in hardware; read the position with `__HAL_TIM_GET_COUNTER()`.

**Listing 3.7 — 1 kHz square wave from output compare (toggle), 1 µs tick**

```c
/* TIM3 CH1 output compare toggle, PSC = 71, ARR = 0xFFFF, NVIC enabled */
#define HALF_PERIOD_TICKS 500U

void OC_Start(void) { HAL_TIM_OC_Start_IT(&htim3, TIM_CHANNEL_1); }

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        uint32_t next = __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1) + HALF_PERIOD_TICKS;
        __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, next & 0xFFFFU);
    }
}
```

### 3.6.6 Timer trigger output (TRGO), interrupts and DMA

- **TRGO.** The master-mode selection (`MMS` in `CR2`) chooses what appears on TRGO: reset, enable, **update**, compare pulse, compare channels. An ADC (or DAC) configured with an **external trigger** of that timer’s TRGO starts one conversion per event, with no software jitter. The ADC’s allowed trigger list is family- and instance-specific; a TRGO that is not on the list will silently never sample.
- **Timer interrupts.** Update, capture/compare, trigger and break events each have their own enable bit and callback.
- **Timer DMA requests.** An update or compare event can request DMA, for example to stream a table into `CCRx` and generate an arbitrary waveform or drive addressable LEDs with `HAL_TIM_PWM_Start_DMA()`.

### 3.6.7 Timer pitfalls

- Assuming the timer clock equals SYSCLK or PCLK (Section 2.7).
- Off-by-one: registers divide by *value + 1*.
- Interrupt not enabled in NVIC, or the callback filters the wrong `Instance`.
- Timer started with the wrong function (`Base_Start` instead of `PWM_Start`).
- Pin not configured with the timer’s alternate function.
- Timer already used as the HAL time base or by another subsystem (TIM1 for HAL tick, for instance).
- Calling blocking UART output from the timer callback.
- An LED that is active-low giving the impression of inverted duty.

### 3.6.8 Choosing a timer feature

| Requirement | Feature |
| --- | --- |
| 1 ms periodic interrupt | Basic timer or general timer with update interrupt |
| 20 kHz PWM | Output-compare PWM; mind resolution = ARR + 1 |
| Measure external frequency or pulse width | Input capture (PWM-input mode for duty) |
| Motor-control PWM with dead time | Advanced-control timer |
| Accurate timing without blocking | Timer interrupt, output compare, or timer-triggered peripherals |
| Quadrature encoder | Encoder mode |
| Fixed-rate ADC sampling | Timer TRGO → ADC |

### 3.6.9 Watchdogs, RTC and SysTick compared with timers

- **IWDG** runs from its own low-speed clock and resets the MCU if not refreshed in time. **WWDG** additionally requires the refresh to fall *inside a window*; refreshing too early is as wrong as too late.
- **Best practice:** never refresh a watchdog unconditionally from a timer interrupt if the main application can deadlock. Refresh only when the checks that define “healthy” have passed, otherwise the watchdog is decoration.
- **RTC** provides calendar time or a low-power timebase (often from LSE). **General timers** give precise events, PWM and capture. **SysTick** supplies the HAL/RTOS tick and is not a substitute for hardware timer features.

## 3.7 Integrated Example: Timer-Triggered ADC with Circular DMA

This is the most important integrated example because it connects clocks, timers, ADC, DMA, interrupts, buffers and application processing.

**Goal.** Collect analog measurements at a predictable rate while spending almost no CPU per sample.

```text
Timer update (TRGO)  ->  ADC conversion  ->  EOC  ->  DMA request  ->  SRAM buffer
                                                         |
                                                half / full transfer IRQ
                                                         v
                                              application processes a block
```

### 3.7.1 Design decisions before touching CubeMX

1. Required sample rate *F*<sub>s</sub> and the highest signal frequency of interest (Nyquist: *F*<sub>s</sub> > 2 *f*<sub>max</sub>, plus an analog anti-alias filter).
2. Number of ADC channels (scan) and the sample time each needs.
3. Can one conversion (sample time + 12.5 cycles) finish before the next trigger?
4. Buffer size *N* versus latency and RAM, and whether the CPU can process half a buffer before DMA returns to it.
5. Cache maintenance needs (Cortex-M7).
6. How overruns and missed blocks will be detected and reported.

### 3.7.2 Configuration (STM32F103: ADC1 + TIM3 TRGO + DMA1 channel 1)

1. Set the analog pin to **Analog** mode; choose the ADC channel and rank.
2. ADC: 12-bit, right alignment, **Continuous Conversion Mode disabled**, **External Trigger = Timer 3 Trigger Out event** (on F103 the regular-group trigger list contains TIM3_TRGO; TIM2 TRGO is *not* a regular ADC1 trigger on this device, **Verify** in RM0008), **DMA Continuous Requests enabled**, sample time chosen from source impedance (Section 5.2).
3. DMA settings: ADC1 request, peripheral→memory, **circular**, peripheral increment off, memory increment on, half-word/half-word, medium or higher priority. Enable DMA1 channel1 interrupt in NVIC.
4. TIM3: internal clock, PSC and ARR from the sample rate, **Trigger Event Selection = Update Event**.
5. Generate code. In firmware: **calibrate the ADC** (`HAL_ADCEx_Calibration_Start()`; required on F1 and several other families), start `HAL_ADC_Start_DMA()`, then start the timer.

**Sample-rate calculation.** For *F*<sub>s</sub> = 10 kHz from TIM3 at 72 MHz: PSC = 71 (1 MHz), ARR = 99 → 1 MHz / 100 = 10 kHz. Check the conversion time: ADCCLK = 12 MHz, sample time 55.5 cycles → (55.5 + 12.5) / 12 MHz = **5.67 µs**, well below the 100 µs trigger period. For a multi-channel scan, distinguish the trigger rate, the per-channel sample rate (= trigger rate when one trigger starts the whole scan), the time to complete the whole scan and the DMA item rate.

**Listing 3.8 — Timer-triggered ADC with circular DMA, ping-pong processing and overrun counting**

```c
#define ADC_BUF_LEN  200U                 /* 200 samples: 100 per half */

static uint16_t          adc_dma_buffer[ADC_BUF_LEN];
static volatile uint8_t  g_half_ready, g_full_ready;
static volatile uint32_t g_adc_overruns;

void Acquisition_Start(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)              { Error_Handler(); }
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_BUF_LEN) != HAL_OK)
                                                                      { Error_Handler(); }
    if (HAL_TIM_Base_Start(&htim3) != HAL_OK)                       { Error_Handler(); }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        if (g_half_ready) { g_adc_overruns++; }      /* previous half was never processed */
        g_half_ready = 1U;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        if (g_full_ready) { g_adc_overruns++; }
        g_full_ready = 1U;
    }
}

void Acquisition_Process(void)                     /* call from the main loop or an RTOS task */
{
    if (g_half_ready) { g_half_ready = 0U; DSP_ProcessBuffer(&adc_dma_buffer[0],              ADC_BUF_LEN / 2U); }
    if (g_full_ready) { g_full_ready = 0U; DSP_ProcessBuffer(&adc_dma_buffer[ADC_BUF_LEN / 2U], ADC_BUF_LEN / 2U); }
}
```

**Order matters.** Start the ADC+DMA before the timer so that the first trigger finds an armed converter; some families’ application notes specify the opposite for particular modes, **Verify** against your reference manual.

### 3.7.3 The half-buffer deadline

$$ T_{\text{full}} = \frac{N}{F_s} \qquad T_{\text{half}} = \frac{N}{2\,F_s} $$

**Variables.** *N* total samples in the circular buffer (all channels counted if scanning), *F*<sub>s</sub> the item rate in samples per second, *T*<sub>half</sub> the time the CPU has to finish one half before DMA overwrites it.

**Example.** N = 200 at 10 kS/s → *T*<sub>full</sub> = 20 ms, *T*<sub>half</sub> = **10 ms**. If processing 100 samples takes 12 ms the CPU silently mixes old and new samples. Count an `overrun` whenever a half-ready flag is still set when the next half arrives, as the listing does. Ramping the sample rate until `g_adc_overruns` increments tells you the real CPU budget.

### 3.7.4 Verification

Ground the pin (codes near 0, offset error remains) and feed 1.65 V from a divider (codes near mid-scale). Toggle a GPIO in the half callback and confirm the period equals *N*/(2*F*<sub>s</sub>) on a scope. Start at a low rate and increase it gradually while watching overrun counts, ADC overrun and DMA error flags. Never print every sample over a 115 200-baud UART from a 10 kS/s stream (20 kB/s needed against about 11.5 kB/s available).

> [!NOTE]
> **Key takeaway (Part 2).** Interrupts tell the CPU *when*, DMA moves *what*, timers decide *when exactly*. Choose each by CPU budget and deadline, make buffer ownership explicit, count overruns, and measure the real timing.
