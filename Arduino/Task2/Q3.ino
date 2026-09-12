
/*
  Week 02 - Task 3
  HC-SR04 distance measurement with:
  - I2C LCD display
  - Button to toggle cm / inches
  - Potentiometer-controlled LCD backlight PWM
  - Warning LED and blinking LCD above 200 cm
  - Custom degree symbol demonstration

  Button wiring: D2 to button to GND.
  Backlight PWM requires a suitable transistor/driver circuit.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t TRIG_PIN = 8;
const uint8_t ECHO_PIN = 9;
const uint8_t UNIT_BUTTON_PIN = 2;
const uint8_t WARNING_LED_PIN = 3;
const uint8_t BACKLIGHT_PWM_PIN = 5;
const uint8_t POT_PIN = A0;

const float WARNING_DISTANCE_CM = 200.0f;
const unsigned long ECHO_TIMEOUT_US = 30000UL;
const unsigned long MEASURE_INTERVAL_MS = 250;
const unsigned long BLINK_INTERVAL_MS = 500;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Custom degree symbol (5x8 dot matrix).
byte degreeSymbol[8] = {
  B00110,
  B01001,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
  B00000
};

bool showInches = false;
bool previousButtonState = HIGH;
bool lcdVisible = true;
bool warningLedState = false;

unsigned long lastMeasureTime = 0;
unsigned long lastBlinkTime = 0;

float distanceCm = -1.0f;

float readDistanceCm()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  const unsigned long duration =
    pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  if (duration == 0) {
    return -1.0f; // No valid echo
  }

  // Speed of sound is approximately 0.0343 cm per microsecond.
  return duration * 0.0343f / 2.0f;
}

void updateBacklight()
{
  const int potValue = analogRead(POT_PIN);
  const int brightness = map(potValue, 0, 1023, 0, 255);

  analogWrite(BACKLIGHT_PWM_PIN, constrain(brightness, 0, 255));
}

bool stableButtonState = HIGH;
bool lastRawButtonState = HIGH;
unsigned long lastButtonChangeTime = 0;
const unsigned long DEBOUNCE_MS = 40;

void updateButton()
{
  const bool rawState = digitalRead(UNIT_BUTTON_PIN);
  const unsigned long now = millis();

  if (rawState != lastRawButtonState) {
    lastRawButtonState = rawState;
    lastButtonChangeTime = now;
  }

  if (now - lastButtonChangeTime >= DEBOUNCE_MS &&
      rawState != stableButtonState) {
    stableButtonState = rawState;

    // Toggle only when the debounced button becomes pressed.
    if (stableButtonState == LOW) {
      showInches = !showInches;
    }
  }
}

void updateWarning()
{
  const bool warningActive =
    (distanceCm > WARNING_DISTANCE_CM);

  if (!warningActive) {
    warningLedState = false;
    lcdVisible = true;
    digitalWrite(WARNING_LED_PIN, LOW);
    lcd.display();
    return;
  }

  const unsigned long now = millis();

  if (now - lastBlinkTime >= BLINK_INTERVAL_MS) {
    lastBlinkTime = now;
    warningLedState = !warningLedState;
    lcdVisible = !lcdVisible;
  }

  digitalWrite(WARNING_LED_PIN, warningLedState ? HIGH : LOW);

  if (lcdVisible) {
    lcd.display();
  } else {
    lcd.noDisplay();
  }
}

void displayDistance()
{
  if (distanceCm < 0.0f) {
    lcd.setCursor(0, 0);
    lcd.print(F("No echo         "));
    lcd.setCursor(0, 1);
    lcd.print(F("Check sensor    "));
    return;
  }

  const float displayedDistance =
    showInches ? distanceCm / 2.54f : distanceCm;

  lcd.setCursor(0, 0);

  if (distanceCm > WARNING_DISTANCE_CM) {
    lcd.print(F("WARNING >200cm  "));
  } else {
    lcd.print(F("Distance:       "));
  }

  lcd.setCursor(0, 1);
  lcd.print(displayedDistance, 1);

  if (showInches) {
    lcd.print(F(" inch       "));
  } else {
    lcd.print(F(" cm          "));
  }
}

void printSerial()
{
  Serial.print(F("Distance: "));

  if (distanceCm < 0.0f) {
    Serial.println(F("No echo"));
    return;
  }

  Serial.print(distanceCm, 1);
  Serial.print(F(" cm | "));

  Serial.print(distanceCm / 2.54f, 1);
  Serial.println(F(" inches"));
}

void setup()
{
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(UNIT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(WARNING_LED_PIN, OUTPUT);
  pinMode(BACKLIGHT_PWM_PIN, OUTPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(WARNING_LED_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degreeSymbol);

  // Demonstrate the custom degree symbol.
  lcd.setCursor(0, 0);
  lcd.print(F("Distance sensor"));
  lcd.setCursor(0, 1);
  lcd.write(byte(0));
  lcd.print(F("C ready"));
  delay(1000);
  lcd.clear();
}

void loop()
{
  updateButton();
  updateBacklight();

  const unsigned long now = millis();

  if (now - lastMeasureTime >= MEASURE_INTERVAL_MS) {
    lastMeasureTime = now;
    distanceCm = readDistanceCm();
    printSerial();
  }

  updateWarning();

  // Update text only when the LCD is currently visible.
  if (lcdVisible) {
    displayDistance();
  }
}