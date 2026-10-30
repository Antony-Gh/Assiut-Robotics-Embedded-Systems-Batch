# AQUASYNC ROV Navigation & Multi-Sensor Telemetry Suite
### Target: STM32F401RCT6 (ARM Cortex-M4 @ 84MHz)

---

## 1. Hardware Pinout & Wiring Architecture

All pin definitions, peripheral handles, and bus parameters are centralized in [`Core/Inc/sensor_config.h`](file:///e:/Robotics_Team/ROV/TestingCodes/STM32F401RCT6/STM32F401RCT6/Core/Inc/sensor_config.h). Changing pins takes just 1 line of code.

### Connection Matrix

| Sensor Board | Sensor Pin | STM32F401RCT6 Pin | Function / Alternate Function | Notes & Configuration |
| :--- | :--- | :--- | :--- | :--- |
| **Common I2C Bus** | **SCL** | **PB8** | I2C1_SCL (AF4, Open Drain, Pull-up) | 400 kHz Fast Mode |
| **Common I2C Bus** | **SDA** | **PB9** | I2C1_SDA (AF4, Open Drain, Pull-up) | 400 kHz Fast Mode |
| **GY-87** | *(No AD0 pin)* | **Internal GND** | Onboard MPU-6050 Address | Fixed at `0x68` (AD0 is tied to GND on the PCB) |
| **GY-521** | **AD0** | **3.3V (VCC)** | Hardware Address Pin | Sets GY-521 to `0x69` (prevents bus collision!) |
| **Both Modules**| **VCC / GND** | **3.3V / GND** | Power Supply | 3.3V regulated power rail |
| **MS5540C** | **MCLK** | **PA0** | TIM2_CH1 (PWM 32.768 kHz, 50% duty) | Continuous hardware clock for ADC |
| **MS5540C** | **SCLK** | **PB13** | GPIO Output Push-Pull | 3-Wire Synchronous Serial Clock |
| **MS5540C** | **DIN** | **PB15** | GPIO Output Push-Pull (Master OUT) | Connects to MS5540C DIN |
| **MS5540C** | **DOUT** | **PB14** | GPIO Input Pull-up (Master IN) | Connects to MS5540C DOUT |
| **Telemetry UART** | **TX** | **PA2** | USART2_TX (AF7) with **DMA1 Stream 6** | Connects to USB-TTL / Arduino / PC |
| **Telemetry UART** | **RX** | **PA3** | USART2_RX (AF7) | Command & configuration input |

---

## 2. Software Architecture & Drivers

```
STM32F401RCT6/
├── Core/
│   ├── Inc/
│   │   ├── sensor_config.h    // Centralized hardware pins, I2C addresses, and baud rates
│   │   ├── mpu6050.h          // GY-521 & GY-87 6-DOF IMU driver with DLPF & complementary filter
│   │   ├── hmc5883l.h         // Auto-detecting driver for authentic HMC5883L & QMC5883L clones
│   │   ├── bmp180.h           // Bosch BMP180 Barometer & Altimeter (on GY-87)
│   │   ├── gy87.h             // 10-DOF composite manager (handles MPU-6050 I2C bypass mode)
│   │   ├── ms5540c.h          // High-precision depth/pressure driver with 2nd-order temp comp
│   │   └── logger.h           // Non-blocking DMA ring-buffer logger & JSON streamer
│   └── Src/
│       ├── mpu6050.c
│       ├── hmc5883l.c
│       ├── bmp180.c
│       ├── gy87.c
│       ├── ms5540c.c
│       ├── logger.c
│       ├── main.c             // 20Hz-50Hz real-time telemetry loop
│       └── stm32f4xx_it.c     // DMA1 Stream 6 & USART2 interrupt handlers
```

---

## 3. Key Driver Engineering Highlights

### A. Non-Blocking DMA Ring-Buffer Logger (`logger.c`)
- **Zero CPU Starvation**: Formats log messages into a memory ring buffer (< 1 µs execution).
- **Autonomous Streaming**: Uses **DMA1 Stream 6 Channel 4** to pump bytes directly from SRAM to the USART transmitter in hardware.
- Emits structured JSON streaming frames formatted for the Web Dashboard:
  ```json
  {"ts":14250,"gy521":{"ax":0.02,"ay":0.01,"az":0.99,"gx":0.1,"gy":-0.2,"gz":0.0,"roll":0.8,"pitch":1.2},"gy87":{"ax":0.01,"ay":0.00,"az":1.00,"gx":0.0,"gy":0.0,"gz":0.1,"mx":15.2,"my":-8.4,"mz":41.0,"hdg":142.5,"press":1013.2,"alt":0.0},"ms5540":{"press":1120.4,"temp":21.8,"depth":1.07}}
  ```

### B. GY-87 I2C Master Bypass
- GY-87 has the MPU-6050, HMC5883L, and BMP180 on one board.
- The driver configures `INT_PIN_CFG (0x37)` with `BYPASS_EN = 1` and clears `I2C_MST_EN` in `USER_CTRL`, directly bridging the internal auxiliary bus to the main I2C bus so the STM32 can address all 3 chips simultaneously.

### C. HMC5883L vs QMC5883L Auto-Detection
- In robotics competitions, 90% of breakout boards labeled "HMC5883L" actually contain the **QMC5883L** clone (different I2C address `0x0D` vs `0x1E`, inverted register layout, little-endian data).
- The driver probes both chips on startup and transparently configures the correct register mapping.

### D. MS5540C Depth & Pressure Engine
- **MCLK**: Generates a continuous **32.768 kHz square wave (50% duty cycle)** using TIM2 PWM on PA0 with zero CPU overhead.
- **21-bit Reset**: Sends the exact 21-bit pattern (`101010101010101010101`) specified in the Measurement Specialties datasheet.
- **Factory Calibration**: Extracts 6 PROM coefficients (C1 to C6).
- **2nd-Order Low-Temperature Compensation**: Corrects for non-linearities when diving below 20.0 °C.
- **Depth Calculation**: Computes depth in meters using hydrostatic fluid mechanics ($Depth = \Delta P / (\rho \cdot g)$).

---

## 4. Modern Cybernetic Web Dashboard (WebSerial)

Located in [`Dashboard/`](file:///e:/Robotics_Team/ROV/TestingCodes/STM32F401RCT6/Dashboard/):
- **`index.html`**: Semantic HTML structure with HUD widgets.
- **`style.css`**: Deep navy/slate cybernetic theme, glassmorphism, glowing meters, responsive layout.
- **`app.js`**: Web Serial API engine, 60 FPS Canvas 3D Artificial Horizon, real-time oscilloscope, compass dial, depth column animation, and session recorder.

### How to Run the Dashboard:
1. Double-click [`Dashboard/index.html`](file:///e:/Robotics_Team/ROV/TestingCodes/STM32F401RCT6/Dashboard/index.html) in Google Chrome, Microsoft Edge, or Opera.
2. Select your baud rate (default: `115200`).
3. Click **"Connect Serial"** and choose your STM32 USB-UART / ST-Link VCP port.
4. *No hardware plugged in yet?* Simply toggle **"Demo Simulation"** on the top bar to preview real-time 3D horizon pitching/rolling, depth gauge animation, compass rotation, and oscilloscope plotting!