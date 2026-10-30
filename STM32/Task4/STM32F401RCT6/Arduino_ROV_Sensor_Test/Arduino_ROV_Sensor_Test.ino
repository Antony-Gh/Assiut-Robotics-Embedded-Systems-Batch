/**
 * ==============================================================================
 * AQUASYNC ROV TELEMETRY & MULTI-SENSOR TEST SUITE (ARDUINO / STM32DUINO)
 * ==============================================================================
 * Compatible with: Arduino Uno/Nano/Mega/ESP32 OR STM32F401 (Black Pill) via
 * STM32duino
 *
 * Sensors Supported:
 *   1. GY-521   (MPU-6050 6-DOF IMU: Accel + Gyro + Temperature + Pitch/Roll)
 *   2. GY-87    (10-DOF Composite: MPU-6050 + HMC5883L/QMC5883L Mag + BMP180
 * Baro)
 *   3. MS5540C  (Subsea Depth & Pressure Sensor: MCLK, SCLK, DIN, DOUT)
 *
 * Features:
 *   - Standalone: Requires ZERO external libraries! (Uses only standard Wire.h)
 *   - Built-in I2C Scanner to immediately test breadboard wiring and addresses.
 *   - Per-Sensor switches: Enable / Disable each sensor individually to test
 * one by one.
 *   - Streams JSON directly into the AQUASYNC Web Dashboard
 * (http://localhost:8080/).
 *   - Human-readable Serial Monitor test mode.
 * ==============================================================================
 */

#include <Wire.h>
#include <math.h>

// ==============================================================================
// 1. SENSOR CONFIGURATION & TEST SWITCHES (Enable/Disable to test one by one!)
// ==============================================================================
#define ENABLE_GY521 1   // 1 = Test GY-521 (MPU-6050)
#define ENABLE_GY87 0    // 1 = Test GY-87 (10-DOF Composite)
#define ENABLE_MS5540C 0 // 1 = Test MS5540C Subsea Depth Sensor

// Output Mode:
//   1 = JSON stream for AQUASYNC Web Dashboard (115200 baud)
//   0 = Human-Readable Text in Arduino Serial Monitor
#define OUTPUT_JSON_FOR_DASHBOARD 0

// Stream rate (Hz)
#define TELEMETRY_RATE_HZ 20

// I2C Addresses
#define GY521_I2C_ADDR 0x68    // 0x68 (AD0=GND/float) or 0x69 (AD0=3.3V)
#define GY87_MPU_ADDR 0x68     // Fixed at 0x68 on GY-87
#define HMC5883L_I2C_ADDR 0x1E // Honeywell Compass
#define QMC5883L_I2C_ADDR 0x0D // QMC Compass clone
#define BMP180_I2C_ADDR 0x77   // BMP180 Barometer

// Local Magnetic Declination in degrees (adjust for your region)
#define MAGNETIC_DECLINATION_DEG 4.5f

// ==============================================================================
// 2. PIN DEFINITIONS
// ==============================================================================
/*
 * I2C Pins:
 *   Arduino Uno / Nano: SCL = A5, SDA = A4
 *   Arduino Mega:       SCL = 21, SDA = 20
 *   ESP32:              SCL = 22, SDA = 21 (or default)
 *   STM32 Black Pill:   SCL = PB8 (or PB6), SDA = PB9 (or PB7)
 *
 * MS5540C 3-Wire + MCLK Pins (Change to your preferred digital pins):
 */
#if defined(ARDUINO_ARCH_STM32)
#define MS5540C_MCLK_PIN PA0 // 32.768 kHz continuous clock
#define MS5540C_SCLK_PIN PB13
#define MS5540C_DIN_PIN PB15  // Master OUT -> Sensor DIN
#define MS5540C_DOUT_PIN PB14 // Master IN <- Sensor DOUT
#else
#define MS5540C_MCLK_PIN 9 // 32.768 kHz clock output
#define MS5540C_SCLK_PIN 4
#define MS5540C_DIN_PIN 5  // MOSI
#define MS5540C_DOUT_PIN 6 // MISO
#endif

// ==============================================================================
// 3. DATA STRUCTURES
// ==============================================================================
struct MPU_Data {
  bool active = false;
  float ax = 0, ay = 0, az = 1; // g
  float gx = 0, gy = 0, gz = 0; // deg/s
  float temp = 0;               // deg C
  float roll = 0, pitch = 0;    // deg (Complementary filter)
};

struct Mag_Data {
  bool active = false;
  bool is_qmc = false;
  float mx = 0, my = 0, mz = 0; // uT
  float heading = 0;            // deg
};

struct Baro_Data {
  bool active = false;
  float pressure_hpa = 1013.25;
  float temp = 0;
  float altitude = 0;
};

struct Depth_Data {
  bool active = false;
  float pressure_mbar = 1013.25;
  float temp = 22.0;
  float depth_m = 0.0;
  float surface_baseline_mbar = 1013.25;
  uint16_t c1, c2, c3, c4, c5, c6;
};

MPU_Data data_gy521;
MPU_Data data_gy87_mpu;
Mag_Data data_gy87_mag;
Baro_Data data_gy87_baro;
Depth_Data data_ms5540;

// BMP180 EEPROM Calibration
struct {
  int16_t ac1, ac2, ac3;
  uint16_t ac4, ac5, ac6;
  int16_t b1, b2, mb, mc, md;
} bmp180_cal;

// Timing variables
unsigned long last_telemetry_ms = 0;
unsigned long last_attitude_ms = 0;
unsigned long last_baro_ms = 0;
unsigned long last_depth_ms = 0;
unsigned long last_diag_ms = 0;

// ==============================================================================
// 4. LOW-LEVEL I2C HELPER FUNCTIONS
// ==============================================================================
uint8_t writeReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t val) {
  Wire.beginTransmission(dev_addr);
  Wire.write(reg_addr);
  Wire.write(val);
  return Wire.endTransmission();
}

bool readRegs(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(dev_addr);
  Wire.write(reg_addr);
  if (Wire.endTransmission(false) != 0)
    return false;

  uint8_t count = Wire.requestFrom((int)dev_addr, (int)len);
  if (count != len)
    return false;
  for (uint8_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }
  return true;
}

// ==============================================================================
// 5. I2C BUS SCANNER & HARDWARE LINE DIAGNOSTICS
// ==============================================================================
void checkI2CPhysicalLines() {
  Serial.println(F("----------------------------------------"));
  Serial.println(F(" [HARDWARE VOLTMETER] SCL / SDA Voltage:"));
#if defined(__AVR__)
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  delay(10);
  int raw_scl = analogRead(A5);
  int raw_sda = analogRead(A4);
  float v_scl = (float)raw_scl * 5.0f / 1023.0f;
  float v_sda = (float)raw_sda * 5.0f / 1023.0f;

  Serial.print(F("   - SCL (Pin A5): "));
  Serial.print(v_scl, 2);
  Serial.print(F(" V  "));
  if (v_scl < 0.3f) {
    Serial.println(F("[SHORT TO GND / 0V]"));
  } else if (v_scl < 1.5f) {
    Serial.println(F("[CLAMPED LOW / UNPOWERED SENSOR]"));
  } else if (v_scl >= 2.8f) {
    Serial.println(F("[HEALTHY HIGH]"));
  } else {
    Serial.println(F("[FLOATING]"));
  }

  Serial.print(F("   - SDA (Pin A4): "));
  Serial.print(v_sda, 2);
  Serial.print(F(" V  "));
  if (v_sda < 0.3f) {
    Serial.println(F("[SHORT TO GND / 0V]"));
  } else if (v_sda < 1.5f) {
    Serial.println(F("[CLAMPED LOW / UNPOWERED SENSOR]"));
  } else if (v_sda >= 2.8f) {
    Serial.println(F("[HEALTHY HIGH]"));
  } else {
    Serial.println(F("[FLOATING]"));
  }

  // Also test with internal pullup enabled
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  delay(10);
  int raw_scl_pu = analogRead(A5);
  int raw_sda_pu = analogRead(A4);
  float v_scl_pu = (float)raw_scl_pu * 5.0f / 1023.0f;
  float v_sda_pu = (float)raw_sda_pu * 5.0f / 1023.0f;

  Serial.print(F("   - With Internal Pullup -> SCL: "));
  Serial.print(v_scl_pu, 2);
  Serial.print(F(" V | SDA: "));
  Serial.print(v_sda_pu, 2);
  Serial.println(F(" V"));
#else
  Serial.println(F("   - Hardware line check enabled"));
#endif
  Serial.println(F("----------------------------------------"));
}

void i2cBusRecovery() {
#if defined(__AVR__)
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, OUTPUT);
  // Send 9 clock pulses on SCL to release any hung slave
  for (int i = 0; i < 9; i++) {
    digitalWrite(A5, LOW);
    delayMicroseconds(10);
    digitalWrite(A5, HIGH);
    delayMicroseconds(10);
  }
  // Generate a clean STOP condition
  pinMode(A4, OUTPUT);
  digitalWrite(A4, LOW);
  delayMicroseconds(10);
  digitalWrite(A5, HIGH);
  delayMicroseconds(10);
  digitalWrite(A4, HIGH);
  delayMicroseconds(10);
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
#endif
}

void scanI2C(bool verbose = false) {
  Serial.println(F("\n========================================"));
  Serial.println(F("   AQUASYNC I2C Bus Diagnostic Scanner  "));
  Serial.println(F("========================================"));
  byte count = 0;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte err = Wire.endTransmission();

    if (err == 0) {
      count++;
      Serial.print(F(" [FOUND] 0x"));
      if (addr < 16)
        Serial.print(F("0"));
      Serial.print(addr, HEX);
      Serial.print(F(" -> "));

      switch (addr) {
      case 0x68:
        Serial.println(F("MPU-6050 (GY-521 AD0=GND or GY-87 MPU)"));
        break;
      case 0x69:
        Serial.println(F("MPU-6050 (GY-521 AD0=3.3V)"));
        break;
      case 0x1E:
        Serial.println(F("HMC5883L 3-Axis Magnetometer"));
        break;
      case 0x0D:
        Serial.println(F("QMC5883L 3-Axis Magnetometer (Clone)"));
        break;
      case 0x77:
        Serial.println(F("BMP180 / BMP085 Barometer"));
        break;
      case 0x76:
        Serial.println(F("BMP280 / BME280 Alt Barometer"));
        break;
      default:
        Serial.println(F("Unknown I2C Device"));
        break;
      }
    } else {
      bool found_via_who = false;
      // If 0x68 or 0x69 failed the 0-byte ping, try probing WHO_AM_I (0x75) directly!
      if (addr == 0x68 || addr == 0x69) {
        Wire.beginTransmission(addr);
        Wire.write(0x75);
        if (Wire.endTransmission(false) == 0) {
          if (Wire.requestFrom((int)addr, 1) == 1) {
            uint8_t who = Wire.read();
            count++;
            found_via_who = true;
            Serial.print(F(" [FOUND VIA REGISTER] 0x"));
            Serial.print(addr, HEX);
            Serial.print(F(" -> MPU-6050 WHO_AM_I = 0x"));
            Serial.print(who, HEX);
            Serial.println(F(" (Responds to Register Reads!)"));
          }
        }
      }

      if (!found_via_who) {
        bool is_target = (addr == 0x68 || addr == 0x69 || addr == 0x1E ||
                          addr == 0x0D || addr == 0x77 || addr == 0x76);

        // Report target addresses, bus errors (err 4), or all if verbose
        if (err == 4 || is_target || verbose) {
          Serial.print(F(" [NOT FOUND] 0x"));
          if (addr < 16)
            Serial.print(F("0"));
          Serial.print(addr, HEX);
        if (is_target) {
          switch (addr) {
          case 0x68:
            Serial.print(F(" (MPU-6050 AD0=0)"));
            break;
          case 0x69:
            Serial.print(F(" (MPU-6050 AD0=1)"));
            break;
          case 0x1E:
            Serial.print(F(" (HMC5883L Mag) "));
            break;
          case 0x0D:
            Serial.print(F(" (QMC5883L Mag) "));
            break;
          case 0x77:
            Serial.print(F(" (BMP180 Baro)  "));
            break;
          case 0x76:
            Serial.print(F(" (BMP280 Baro)  "));
            break;
          }
        }
        Serial.print(F(" -> Err "));
        Serial.print(err);
        Serial.print(F(": "));
        switch (err) {
        case 1:
          Serial.println(F("Data buffer overflow"));
          break;
        case 2:
          Serial.println(F("NACK (No device response. Check VCC/GND/AD0/Wiring)"));
          break;
        case 3:
          Serial.println(F("NACK on data byte (Device rejected register)"));
          break;
        case 4:
          Serial.println(F("BUS ERROR (SCL/SDA lines shorted or noisy)"));
          break;
        case 5:
          Serial.println(F("TIMEOUT (Clock stretching hung)"));
          break;
        default:
          Serial.println(F("Communication failed"));
          break;
        }
      }
    }
  }
}

  if (count == 0) {
    Serial.println(F("\n [DIAGNOSTIC SUMMARY: 0 Devices Found]"));
    Serial.println(F(" 1. Check Module VCC: Must have 3.3V or 5V (measure with multimeter if possible)."));
    Serial.println(F(" 2. Check Module GND: Must share common ground with Arduino GND pin."));
    Serial.println(F(" 3. Check SCL Wire: Connect to Uno Pin A5 (NOT digital pin 5)."));
    Serial.println(F(" 4. Check SDA Wire: Connect to Uno Pin A4 (NOT digital pin 4)."));
    Serial.println(F(" 5. Breadboard contact: Jumper wires on breadboards often have loose clips."));
  } else {
    Serial.print(F("\n Total devices discovered: "));
    Serial.println(count);
  }
  Serial.println(F("========================================\n"));
}

// ==============================================================================
// 6. MPU-6050 DRIVER (GY-521 / GY-87)
// ==============================================================================
bool initMPU6050(uint8_t addr, bool enable_bypass) {
  // Step 1: Probe address
  Wire.beginTransmission(addr);
  byte ping_err = Wire.endTransmission();
  if (ping_err != 0) {
    Serial.print(F("\n   [ERR] Ping to MPU @ 0x"));
    Serial.print(addr, HEX);
    Serial.print(F(" failed with Wire code "));
    Serial.print(ping_err);
    if (ping_err == 2)
      Serial.println(F(" (NACK: Chip not answering. Check VCC/GND/AD0 wiring)"));
    else if (ping_err == 4)
      Serial.println(F(" (BUS ERROR: I2C line stuck low or noise)"));
    else
      Serial.println(F(")"));
    return false;
  }

  // Step 2: Read WHO_AM_I register (0x75)
  uint8_t who = 0;
  if (!readRegs(addr, 0x75, &who, 1)) {
    Serial.println(F("\n   [ERR] Could not read WHO_AM_I register (0x75)."));
    return false;
  }
  Serial.print(F(" [WHO_AM_I=0x"));
  Serial.print(who, HEX);
  Serial.print(F("] "));
  if (who != 0x68 && who != 0x70 && who != 0x72) {
    Serial.print(F("-> WARNING: Expected 0x68, got 0x"));
    Serial.print(who, HEX);
    Serial.println(F(" (Check AD0 pin state)"));
  }

  // Step 3: Reset chip
  uint8_t rerr = writeReg(addr, 0x6B, 0x80);
  if (rerr != 0) {
    Serial.print(F("\n   [ERR] Failed writing reset to 0x6B (Wire code "));
    Serial.print(rerr);
    Serial.println(F(")"));
    return false;
  }
  delay(50);

  // Step 4: Wake up with PLL on X gyro
  uint8_t werr = writeReg(addr, 0x6B, 0x01);
  if (werr != 0) {
    Serial.print(F("\n   [ERR] Failed waking MPU from sleep (Wire code "));
    Serial.print(werr);
    Serial.println(F(")"));
    return false;
  }
  delay(10);

  // Step 5: Configure filters and ranges
  writeReg(addr, 0x1A, 0x03); // DLPF = 44 Hz
  writeReg(addr, 0x19, 0x00); // Sample rate divider = 0
  writeReg(addr, 0x1B, 0x08); // Gyro: +/- 500 deg/s
  writeReg(addr, 0x1C, 0x08); // Accel: +/- 4g

  if (enable_bypass) {
    writeReg(addr, 0x6A, 0x00); // Clear USER_CTRL
    writeReg(addr, 0x37, 0x02); // Set BYPASS_EN in INT_PIN_CFG
    Serial.print(F("[BYPASS ENABLED] "));
  }

  return true;
}

bool readMPU6050(uint8_t addr, MPU_Data &data, float dt) {
  uint8_t buf[14];
  if (!readRegs(addr, 0x3B, buf, 14))
    return false;

  int16_t raw_ax = (buf[0] << 8) | buf[1];
  int16_t raw_ay = (buf[2] << 8) | buf[3];
  int16_t raw_az = (buf[4] << 8) | buf[5];
  int16_t raw_t = (buf[6] << 8) | buf[7];
  int16_t raw_gx = (buf[8] << 8) | buf[9];
  int16_t raw_gy = (buf[10] << 8) | buf[11];
  int16_t raw_gz = (buf[12] << 8) | buf[13];

  data.ax = (float)raw_ax / 8192.0f;
  data.ay = (float)raw_ay / 8192.0f;
  data.az = (float)raw_az / 8192.0f;

  data.gx = (float)raw_gx / 65.5f;
  data.gy = (float)raw_gy / 65.5f;
  data.gz = (float)raw_gz / 65.5f;

  data.temp = ((float)raw_t / 340.0f) + 36.53f;

  // Accel angles
  float accel_roll =
      atan2(data.ay, sqrt(data.ax * data.ax + data.az * data.az)) * 57.2958f;
  float accel_pitch =
      atan2(-data.ax, sqrt(data.ay * data.ay + data.az * data.az)) * 57.2958f;

  // Complementary filter (96% gyro integration, 4% accel vector)
  if (dt > 0.0f && dt < 0.2f) {
    data.roll = 0.96f * (data.roll + data.gx * dt) + 0.04f * accel_roll;
    data.pitch = 0.96f * (data.pitch + data.gy * dt) + 0.04f * accel_pitch;
  } else {
    data.roll = accel_roll;
    data.pitch = accel_pitch;
  }

  return true;
}

// ==============================================================================
// 7. MAGNETOMETER DRIVER (HMC5883L & QMC5883L)
// ==============================================================================
bool initMagnetometer(Mag_Data &mag) {
  // 1. Try Honeywell HMC5883L at 0x1E
  uint8_t id[3] = {0};
  if (readRegs(HMC5883L_I2C_ADDR, 10, id, 3)) {
    if (id[0] == 0x48 && id[1] == 0x34 && id[2] == 0x33) {
      writeReg(HMC5883L_I2C_ADDR, 0x00, 0x78); // 8-avg, 75Hz
      writeReg(HMC5883L_I2C_ADDR, 0x01, 0x20); // 1.3 Gauss range
      writeReg(HMC5883L_I2C_ADDR, 0x02, 0x00); // Continuous mode
      mag.is_qmc = false;
      mag.active = true;
      return true;
    }
  }

  // 2. Try QMC5883L Clone at 0x0D
  Wire.beginTransmission(QMC5883L_I2C_ADDR);
  if (Wire.endTransmission() == 0) {
    writeReg(QMC5883L_I2C_ADDR, 0x0A, 0x80); // Soft reset
    delay(10);
    writeReg(QMC5883L_I2C_ADDR, 0x0B, 0x01); // Set period
    writeReg(QMC5883L_I2C_ADDR, 0x09, 0x1D); // 200Hz, 8G range, continuous
    mag.is_qmc = true;
    mag.active = true;
    return true;
  }

  return false;
}

bool readMagnetometer(Mag_Data &mag) {
  if (!mag.active)
    return false;
  uint8_t buf[6];

  if (!mag.is_qmc) {
    // HMC5883L: X MSB, X LSB, Z MSB, Z LSB, Y MSB, Y LSB
    if (!readRegs(HMC5883L_I2C_ADDR, 0x03, buf, 6))
      return false;
    int16_t rx = (buf[0] << 8) | buf[1];
    int16_t rz = (buf[2] << 8) | buf[3];
    int16_t ry = (buf[4] << 8) | buf[5];
    mag.mx = (float)rx * 0.092f;
    mag.my = (float)ry * 0.092f;
    mag.mz = (float)rz * 0.092f;
  } else {
    // QMC5883L: X LSB, X MSB, Y LSB, Y MSB, Z LSB, Z MSB
    if (!readRegs(QMC5883L_I2C_ADDR, 0x00, buf, 6))
      return false;
    int16_t rx = (buf[1] << 8) | buf[0];
    int16_t ry = (buf[3] << 8) | buf[2];
    int16_t rz = (buf[5] << 8) | buf[4];
    mag.mx = (float)rx / 30.0f;
    mag.my = (float)ry / 30.0f;
    mag.mz = (float)rz / 30.0f;
  }

  // Heading calculation
  float heading = atan2(mag.my, mag.mx) * 57.2958f + MAGNETIC_DECLINATION_DEG;
  if (heading < 0)
    heading += 360.0f;
  if (heading >= 360.0f)
    heading -= 360.0f;
  mag.heading = heading;

  return true;
}

// ==============================================================================
// 8. BMP180 BAROMETER DRIVER
// ==============================================================================
bool initBMP180() {
  uint8_t id = 0;
  if (!readRegs(BMP180_I2C_ADDR, 0xD0, &id, 1) || id != 0x55)
    return false;

  uint8_t buf[22];
  if (!readRegs(BMP180_I2C_ADDR, 0xAA, buf, 22))
    return false;

  bmp180_cal.ac1 = (buf[0] << 8) | buf[1];
  bmp180_cal.ac2 = (buf[2] << 8) | buf[3];
  bmp180_cal.ac3 = (buf[4] << 8) | buf[5];
  bmp180_cal.ac4 = (buf[6] << 8) | buf[7];
  bmp180_cal.ac5 = (buf[8] << 8) | buf[9];
  bmp180_cal.ac6 = (buf[10] << 8) | buf[11];
  bmp180_cal.b1 = (buf[12] << 8) | buf[13];
  bmp180_cal.b2 = (buf[14] << 8) | buf[15];
  bmp180_cal.mb = (buf[16] << 8) | buf[17];
  bmp180_cal.mc = (buf[18] << 8) | buf[19];
  bmp180_cal.md = (buf[20] << 8) | buf[21];

  return true;
}

bool readBMP180(Baro_Data &baro) {
  if (!baro.active)
    return false;

  // Trigger temperature
  writeReg(BMP180_I2C_ADDR, 0xF4, 0x2E);
  delay(5);
  uint8_t tbuf[2];
  if (!readRegs(BMP180_I2C_ADDR, 0xF6, tbuf, 2))
    return false;
  int32_t ut = (tbuf[0] << 8) | tbuf[1];

  // Trigger standard pressure
  writeReg(BMP180_I2C_ADDR, 0xF4, 0x34);
  delay(8);
  uint8_t pbuf[3];
  if (!readRegs(BMP180_I2C_ADDR, 0xF6, pbuf, 3))
    return false;
  int32_t up =
      (((int32_t)pbuf[0] << 16) | ((int32_t)pbuf[1] << 8) | pbuf[2]) >> 8;

  // Temperature math
  int32_t x1 = ((ut - (int32_t)bmp180_cal.ac6) * (int32_t)bmp180_cal.ac5) >> 15;
  int32_t x2 = ((int32_t)bmp180_cal.mc << 11) / (x1 + bmp180_cal.md);
  int32_t b5 = x1 + x2;
  baro.temp = ((b5 + 8) >> 4) / 10.0f;

  // Pressure math
  int32_t b6 = b5 - 4000;
  x1 = (bmp180_cal.b2 * ((b6 * b6) >> 12)) >> 11;
  x2 = (bmp180_cal.ac2 * b6) >> 11;
  int32_t x3 = x1 + x2;
  int32_t b3 = ((((int32_t)bmp180_cal.ac1 * 4 + x3)) + 2) >> 2;

  x1 = (bmp180_cal.ac3 * b6) >> 13;
  x2 = (bmp180_cal.b1 * ((b6 * b6) >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  uint32_t b4 = (bmp180_cal.ac4 * (uint32_t)(x3 + 32768)) >> 15;
  uint32_t b7 = ((uint32_t)up - b3) * 50000;

  int32_t p = 0;
  if (b7 < 0x80000000) {
    p = (b7 << 1) / b4;
  } else {
    p = (b7 / b4) << 1;
  }

  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  p = p + ((x1 + x2 + 3791) >> 4);

  baro.pressure_hpa = (float)p / 100.0f;
  baro.altitude =
      44330.0f * (1.0f - pow(baro.pressure_hpa / 1013.25f, 0.1903f));

  return true;
}

// ==============================================================================
// 9. MS5540C SUBSEA DEPTH & PRESSURE DRIVER
// ==============================================================================
void ms5540Pulse() {
  digitalWrite(MS5540C_SCLK_PIN, HIGH);
  delayMicroseconds(3);
  digitalWrite(MS5540C_SCLK_PIN, LOW);
  delayMicroseconds(3);
}

void ms5540SendCmd(uint16_t cmd, uint8_t bits) {
  for (int8_t i = bits - 1; i >= 0; i--) {
    digitalWrite(MS5540C_DIN_PIN, (cmd >> i) & 1 ? HIGH : LOW);
    delayMicroseconds(2);
    ms5540Pulse();
  }
  digitalWrite(MS5540C_DIN_PIN, LOW);
}

uint16_t ms5540ReadWord() {
  uint16_t res = 0;
  for (uint8_t i = 0; i < 16; i++) {
    digitalWrite(MS5540C_SCLK_PIN, HIGH);
    delayMicroseconds(3);
    res = (res << 1) | (digitalRead(MS5540C_DOUT_PIN) == HIGH ? 1 : 0);
    digitalWrite(MS5540C_SCLK_PIN, LOW);
    delayMicroseconds(3);
  }
  return res;
}

bool initMS5540C() {
  pinMode(MS5540C_SCLK_PIN, OUTPUT);
  pinMode(MS5540C_DIN_PIN, OUTPUT);
  pinMode(MS5540C_DOUT_PIN, INPUT_PULLUP);

// Generate 32.768 kHz MCLK if tone() supported
#if defined(tone) || defined(ARDUINO)
  tone(MS5540C_MCLK_PIN, 32768);
#endif

  // 21-bit Reset sequence (0x155555)
  for (int i = 0; i < 21; i++) {
    digitalWrite(MS5540C_DIN_PIN, (i % 2 == 0) ? HIGH : LOW);
    delayMicroseconds(2);
    ms5540Pulse();
  }
  digitalWrite(MS5540C_DIN_PIN, LOW);
  digitalWrite(MS5540C_SCLK_PIN, LOW);
  delay(10);

  // Read PROM Words W1-W4
  ms5540SendCmd(0x1D20, 16);
  uint16_t w1 = ms5540ReadWord();
  ms5540SendCmd(0x1D30, 16);
  uint16_t w2 = ms5540ReadWord();
  ms5540SendCmd(0x1D40, 16);
  uint16_t w3 = ms5540ReadWord();
  ms5540SendCmd(0x1D50, 16);
  uint16_t w4 = ms5540ReadWord();

  data_ms5540.c1 = (w1 >> 3) & 0x1FFF;
  data_ms5540.c2 = ((w1 & 0x07) << 10) | ((w2 >> 6) & 0x03FF);
  data_ms5540.c3 = (w3 >> 6) & 0x03FF;
  data_ms5540.c4 = (w4 >> 7) & 0x01FF;
  data_ms5540.c5 = ((w2 & 0x3F) << 6) | (w3 & 0x3F);
  data_ms5540.c6 = w4 & 0x7F;

  if (data_ms5540.c1 == 0 || data_ms5540.c1 == 0x1FFF) {
    return false;
  }
  return true;
}

bool readMS5540C() {
  if (!data_ms5540.active)
    return false;

  // 1. Request D1 Pressure (0x1D00)
  ms5540SendCmd(0x1D00, 16);
  unsigned long t0 = millis();
  while (digitalRead(MS5540C_DOUT_PIN) == HIGH) {
    if (millis() - t0 > 45)
      return false;
  }
  uint32_t d1 = ms5540ReadWord();

  // 2. Request D2 Temperature (0x1D40)
  ms5540SendCmd(0x1D40, 16);
  t0 = millis();
  while (digitalRead(MS5540C_DOUT_PIN) == HIGH) {
    if (millis() - t0 > 45)
      return false;
  }
  uint32_t d2 = ms5540ReadWord();

  // Calibration compensation
  int32_t ut1 = 8 * (int32_t)data_ms5540.c5 + 20224;
  int32_t dt = (int32_t)d2 - ut1;
  int32_t temp = 200 + ((dt * ((int32_t)data_ms5540.c6 + 50)) >> 10);
  data_ms5540.temp = (float)temp / 10.0f;

  int64_t off = ((int64_t)data_ms5540.c2 * 4) +
                (((int64_t)(data_ms5540.c4 - 512) * dt) >> 12);
  int64_t sens =
      (int64_t)data_ms5540.c1 + (((int64_t)data_ms5540.c3 * dt) >> 10) + 24576;
  int64_t x = (sens * ((int64_t)d1 - 7168)) >> 14;
  int64_t p = (x - off);
  p = (p >> 1) + 2500;

  data_ms5540.pressure_mbar = (float)p / 10.0f;

  // Subsea depth in freshwater (1000 kg/m^3): 1 mbar = ~0.010197 m
  float delta_p = data_ms5540.pressure_mbar - data_ms5540.surface_baseline_mbar;
  data_ms5540.depth_m = max(0.0f, delta_p * 0.010197f);

  return true;
}

// ==============================================================================
// 10. ARDUINO SETUP
// ==============================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000)
    ; // Wait for native USB if present

  Serial.println(F("\n[BOOT] AQUASYNC Subsea Sensor Test Suite Initialized"));

  // 1. Recover bus from any stuck slave state (9 clock pulses)
  i2cBusRecovery();

  // 2. Measure raw line voltages before starting Wire hardware
  checkI2CPhysicalLines();

  // 3. Start Wire hardware with timeout protection
  Wire.begin();
  Wire.setClock(50000); // 50 kHz for maximum margin against noise and weak pull-ups
#if defined(WIRE_HAS_TIMEOUT) || defined(ARDUINO_ARCH_AVR)
  Wire.setWireTimeout(25000, true); // 25ms timeout, auto-reset TWI on timeout (prevents freeze!)
#endif

  // 4. Run I2C bus scan
  scanI2C();

#if ENABLE_GY521
  Serial.print(F("[INIT] Initializing GY-521 (MPU-6050 @ 0x"));
  Serial.print(GY521_I2C_ADDR, HEX);
  Serial.print(F(")... "));
  if (initMPU6050(GY521_I2C_ADDR, false)) {
    data_gy521.active = true;
    Serial.println(F("ONLINE!"));
  } else {
    Serial.println(F("FAILED! Check wiring."));
  }
#endif

#if ENABLE_GY87
  Serial.print(F("[INIT] Initializing GY-87 10-DOF (MPU @ 0x"));
  Serial.print(GY87_MPU_ADDR, HEX);
  Serial.print(F(")... "));
  if (initMPU6050(GY87_MPU_ADDR, true)) { // Enable bypass for Mag & Baro
    data_gy87_mpu.active = true;
    Serial.println(F("ONLINE!"));

    // Magnetometer
    Serial.print(F("[INIT] Detecting GY-87 Magnetometer... "));
    if (initMagnetometer(data_gy87_mag)) {
      Serial.println(data_gy87_mag.is_qmc ? F("QMC5883L ONLINE!")
                                          : F("HMC5883L ONLINE!"));
    } else {
      Serial.println(F("NOT DETECTED"));
    }

    // Barometer
    Serial.print(F("[INIT] Detecting GY-87 BMP180 Barometer... "));
    if (initBMP180()) {
      data_gy87_baro.active = true;
      Serial.println(F("ONLINE!"));
    } else {
      Serial.println(F("NOT DETECTED"));
    }
  } else {
    Serial.println(F("FAILED! Check wiring."));
  }
#endif

#if ENABLE_MS5540C
  Serial.print(F("[INIT] Initializing MS5540C Depth Sensor... "));
  if (initMS5540C()) {
    data_ms5540.active = true;
    delay(50);
    readMS5540C();
    data_ms5540.surface_baseline_mbar = data_ms5540.pressure_mbar;
    Serial.print(F("ONLINE! Surface: "));
    Serial.print(data_ms5540.surface_baseline_mbar, 1);
    Serial.println(F(" mbar"));
  } else {
    Serial.println(F("FAILED! Check PROM wiring."));
  }
#endif

  Serial.println(F("[SYSTEM] Telemetry streaming started at 115200 baud."));
  Serial.println(F(
      "[TIP] Type 's' in Serial Monitor to re-run the I2C Scanner anytime!\n"));
}

// ==============================================================================
// 11. ARDUINO MAIN LOOP
// ==============================================================================
void loop() {
  unsigned long now = millis();

  // Handle Serial input (re-scan bus on 's' or 'v')
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S' || c == 'r' || c == 'R' || c == 'v' || c == 'V') {
      checkI2CPhysicalLines();
      scanI2C();
    }
  }

  // 1. Fast Attitude Update (50 Hz / every 20ms)
  if (now - last_attitude_ms >= 20) {
    float dt = (float)(now - last_attitude_ms) / 1000.0f;
    last_attitude_ms = now;

#if ENABLE_GY521
    if (data_gy521.active) {
      readMPU6050(GY521_I2C_ADDR, data_gy521, dt);
    }
#endif

#if ENABLE_GY87
    if (data_gy87_mpu.active) {
      readMPU6050(GY87_MPU_ADDR, data_gy87_mpu, dt);
    }
#endif
  }

#if ENABLE_GY87
  // 2. Magnetometer & Barometer Update (10 Hz / every 100ms)
  if (now - last_baro_ms >= 100) {
    last_baro_ms = now;
    if (data_gy87_mag.active)
      readMagnetometer(data_gy87_mag);
    if (data_gy87_baro.active)
      readBMP180(data_gy87_baro);
  }
#endif

#if ENABLE_MS5540C
  // 3. MS5540C Depth Update (4 Hz / every 250ms)
  if (now - last_depth_ms >= 250) {
    last_depth_ms = now;
    if (data_ms5540.active)
      readMS5540C();
  }
#endif

  // 4. Telemetry Stream to Web Dashboard (20 Hz / every 50ms)
  if (now - last_telemetry_ms >= (1000 / TELEMETRY_RATE_HZ)) {
    last_telemetry_ms = now;

#if OUTPUT_JSON_FOR_DASHBOARD
    // Send exact JSON packet expected by AQUASYNC Web Dashboard
    Serial.print(F("{\"ts\":"));
    Serial.print(now);

    // GY-521 sub-object
    Serial.print(F(",\"gy521\":{\"ax\":"));
    Serial.print(data_gy521.ax, 2);
    Serial.print(F(",\"ay\":"));
    Serial.print(data_gy521.ay, 2);
    Serial.print(F(",\"az\":"));
    Serial.print(data_gy521.az, 2);
    Serial.print(F(",\"gx\":"));
    Serial.print(data_gy521.gx, 1);
    Serial.print(F(",\"gy\":"));
    Serial.print(data_gy521.gy, 1);
    Serial.print(F(",\"gz\":"));
    Serial.print(data_gy521.gz, 1);
    Serial.print(F(",\"roll\":"));
    Serial.print(data_gy521.roll, 1);
    Serial.print(F(",\"pitch\":"));
    Serial.print(data_gy521.pitch, 1);
    Serial.print(F("}"));

    // GY-87 sub-object
    Serial.print(F(",\"gy87\":{\"ax\":"));
    Serial.print(data_gy87_mpu.ax, 2);
    Serial.print(F(",\"ay\":"));
    Serial.print(data_gy87_mpu.ay, 2);
    Serial.print(F(",\"az\":"));
    Serial.print(data_gy87_mpu.az, 2);
    Serial.print(F(",\"gx\":"));
    Serial.print(data_gy87_mpu.gx, 1);
    Serial.print(F(",\"gy\":"));
    Serial.print(data_gy87_mpu.gy, 1);
    Serial.print(F(",\"gz\":"));
    Serial.print(data_gy87_mpu.gz, 1);
    Serial.print(F(",\"mx\":"));
    Serial.print(data_gy87_mag.mx, 1);
    Serial.print(F(",\"my\":"));
    Serial.print(data_gy87_mag.my, 1);
    Serial.print(F(",\"mz\":"));
    Serial.print(data_gy87_mag.mz, 1);
    Serial.print(F(",\"hdg\":"));
    Serial.print(data_gy87_mag.heading, 1);
    Serial.print(F(",\"press\":"));
    Serial.print(data_gy87_baro.pressure_hpa, 1);
    Serial.print(F(",\"alt\":"));
    Serial.print(data_gy87_baro.altitude, 1);
    Serial.print(F("}"));

    // MS5540C sub-object
    Serial.print(F(",\"ms5540\":{\"press\":"));
    Serial.print(data_ms5540.pressure_mbar, 1);
    Serial.print(F(",\"temp\":"));
    Serial.print(data_ms5540.temp, 1);
    Serial.print(F(",\"depth\":"));
    Serial.print(data_ms5540.depth_m, 2);
    Serial.println(F("}}"));
#endif
  }

  // 5. Human-Readable Periodic Console Readout (Every 1 second)
#if !OUTPUT_JSON_FOR_DASHBOARD
  if (now - last_diag_ms >= 1000) {
    last_diag_ms = now;

#if ENABLE_GY521
    if (data_gy521.active) {
      Serial.print(F("[GY-521] Pitch: "));
      Serial.print(data_gy521.pitch, 1);
      Serial.print(F(" deg | Roll: "));
      Serial.print(data_gy521.roll, 1);
      Serial.print(F(" deg | Accel: "));
      Serial.print(data_gy521.ax, 2);
      Serial.print(F(", "));
      Serial.print(data_gy521.ay, 2);
      Serial.print(F(", "));
      Serial.print(data_gy521.az, 2);
      Serial.print(F(" g | Temp: "));
      Serial.print(data_gy521.temp, 1);
      Serial.println(F(" C"));
    }
#endif

#if ENABLE_GY87
    if (data_gy87_mpu.active) {
      Serial.print(F("[GY-87] HDG: "));
      Serial.print(data_gy87_mag.heading, 1);
      Serial.print(F(" deg | Baro Press: "));
      Serial.print(data_gy87_baro.pressure_hpa, 1);
      Serial.print(F(" hPa | Alt: "));
      Serial.print(data_gy87_baro.altitude, 1);
      Serial.println(F(" m"));
    }
#endif

#if ENABLE_MS5540C
    if (data_ms5540.active) {
      Serial.print(F("[MS5540C] Depth: "));
      Serial.print(data_ms5540.depth_m, 2);
      Serial.print(F(" m | Press: "));
      Serial.print(data_ms5540.pressure_mbar, 1);
      Serial.println(F(" mbar"));
    }
#endif
  }
#endif
}
