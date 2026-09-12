
#include <IRremote.hpp>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// ---------- Hardware ----------
const uint8_t IR_PIN = 2;
const uint8_t LED_PINS[3] = {9, 10, 11}; // Red, Green, Blue

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- Replace with your remote's command values ----------
const uint8_t CMD_NEXT_PAGE = 0x5A;
const uint8_t CMD_PREV_PAGE = 0x08;
const uint8_t CMD_TOGGLE    = 0x1C;
const uint8_t CMD_BRIGHT_UP = 0x18;
const uint8_t CMD_BRIGHT_DOWN = 0x52;

// ---------- Settings ----------
const uint8_t COLOR_COUNT = 3;
const uint8_t BRIGHTNESS_STEP = 16;
const uint8_t EEPROM_MAGIC = 0xA7;
const int EEPROM_MAGIC_ADDRESS = 0;
const int EEPROM_DATA_ADDRESS = 1;

const char *COLOR_NAMES[COLOR_COUNT] = {"Red", "Green", "Blue"};

struct ColorSetting {
  uint8_t brightness; // 0..255
  uint8_t isOn;       // 0 = off, 1 = on
};

ColorSetting colors[COLOR_COUNT];
uint8_t selectedColor = 0;

// ---------- EEPROM ----------
void setDefaultSettings()
{
  for (uint8_t i = 0; i < COLOR_COUNT; i++) {
    colors[i].brightness = 128;
    colors[i].isOn = 1;
  }
}

void saveSettings()
{
  EEPROM.update(EEPROM_MAGIC_ADDRESS, EEPROM_MAGIC);

  for (uint8_t i = 0; i < COLOR_COUNT; i++) {
    const int address = EEPROM_DATA_ADDRESS + i * 2;
    EEPROM.update(address, colors[i].brightness);
    EEPROM.update(address + 1, colors[i].isOn);
  }
}

void loadSettings()
{
  if (EEPROM.read(EEPROM_MAGIC_ADDRESS) != EEPROM_MAGIC) {
    setDefaultSettings();
    saveSettings();
    return;
  }

  for (uint8_t i = 0; i < COLOR_COUNT; i++) {
    const int address = EEPROM_DATA_ADDRESS + i * 2;

    colors[i].brightness = EEPROM.read(address);
    colors[i].isOn = EEPROM.read(address + 1);

    // Recover safely if EEPROM contains unexpected values.
    if (colors[i].isOn > 1) {
      colors[i].isOn = 1;
    }
  }
}

// ---------- LED output ----------
void applyLEDs()
{
  for (uint8_t i = 0; i < COLOR_COUNT; i++) {
    const uint8_t output = colors[i].isOn ? colors[i].brightness : 0;
    analogWrite(LED_PINS[i], output);
  }
}

// ---------- LCD ----------
void showPage()
{
  lcd.clear();

  // Example: "Red ON"
  lcd.setCursor(0, 0);
  lcd.print(COLOR_NAMES[selectedColor]);
  lcd.print(colors[selectedColor].isOn ? F(" ON") : F(" OFF"));

  // Example: "Light: 128 1/3"
  lcd.setCursor(0, 1);
  lcd.print(F("Light: "));
  lcd.print(colors[selectedColor].brightness);
  lcd.print(F(" "));
  lcd.print(selectedColor + 1);
  lcd.print(F("/3"));
}

void setup()
{
  Serial.begin(9600);

  for (uint8_t i = 0; i < COLOR_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }

  lcd.init();
  lcd.backlight();

  loadSettings();
  applyLEDs();
  showPage();

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  Serial.println(F("RGB LED + LCD + EEPROM ready."));
  Serial.println(F("Press remote buttons; note the printed command values."));
}

void loop()
{
  if (!IrReceiver.decode()) {
    return;
  }

  Serial.print(F("IR command: 0x"));
  Serial.println(IrReceiver.decodedIRData.command, HEX);

  // Avoid repeating a toggle or page change while a button is held.
  if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
    const uint8_t command = IrReceiver.decodedIRData.command;
    bool settingsChanged = false;

    switch (command) {
      case CMD_NEXT_PAGE:
        selectedColor = (selectedColor + 1) % COLOR_COUNT;
        break;

      case CMD_PREV_PAGE:
        selectedColor = (selectedColor + COLOR_COUNT - 1) % COLOR_COUNT;
        break;

      case CMD_TOGGLE:
        colors[selectedColor].isOn = !colors[selectedColor].isOn;
        settingsChanged = true;
        break;

      case CMD_BRIGHT_UP:
        colors[selectedColor].brightness =
          (colors[selectedColor].brightness > 255 - BRIGHTNESS_STEP)
          ? 255
          : colors[selectedColor].brightness + BRIGHTNESS_STEP;
        settingsChanged = true;
        break;

      case CMD_BRIGHT_DOWN:
        colors[selectedColor].brightness =
          (colors[selectedColor].brightness < BRIGHTNESS_STEP)
          ? 0
          : colors[selectedColor].brightness - BRIGHTNESS_STEP;
        settingsChanged = true;
        break;

      default:
        // Unknown button: do nothing.
        break;
    }

    if (settingsChanged) {
      saveSettings();
      applyLEDs();
    }

    showPage();
  }

  IrReceiver.resume();
}