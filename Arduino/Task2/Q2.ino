
/*
  Week 02 - Task 2
  LM35 temperature display using two multiplexed
  common-cathode 7-segment digits.

  Segment order: a, b, c, d, e, f, g
  Segment pins: 2, 3, 4, 5, 6, 7, 8
  Digit enable pins: 10 (tens), 11 (ones)

  This code assumes digit-enable transistors are wired so HIGH
  enables the selected digit. Adjust DIGIT_ON if your circuit differs.
*/

const uint8_t TEMP_PIN = A0;

const uint8_t SEGMENT_PINS[7] = {2, 3, 4, 5, 6, 7, 8};
const uint8_t DIGIT_PINS[2] = {10, 11};

const uint8_t GREEN_LED = A1;
const uint8_t YELLOW_LED = A2;
const uint8_t RED_LED = A3;

const uint8_t DIGIT_ON = HIGH;
const uint8_t DIGIT_OFF = LOW;

// Bit order in each pattern: a b c d e f g.
// HIGH lights a segment on a common-cathode display.
const uint8_t DIGIT_PATTERNS[10] = {
  0b1111110, // 0
  0b0110000, // 1
  0b1101101, // 2
  0b1111001, // 3
  0b0110011, // 4
  0b1011011, // 5
  0b1011111, // 6
  0b1110000, // 7
  0b1111111, // 8
  0b1111011  // 9
};

unsigned long lastTemperatureRead = 0;
unsigned long lastBlinkTime = 0;

const unsigned long READ_INTERVAL_MS = 250;
const unsigned long BLINK_INTERVAL_MS = 500;

int displayedTemperature = 0;
bool ledState = false;

void setSegments(uint8_t digit)
{
  const uint8_t pattern = DIGIT_PATTERNS[digit];

  for (uint8_t i = 0; i < 7; i++) {
    const uint8_t bitPosition = 6 - i;
    digitalWrite(SEGMENT_PINS[i],
                 (pattern & (1 << bitPosition)) ? HIGH : LOW);
  }
}

void showNumber(int value)
{
  // Keep the display in the range 00..99.
  value = constrain(value, 0, 99);

  const uint8_t tens = value / 10;
  const uint8_t ones = value % 10;

  // Multiplex one digit at a time.
  digitalWrite(DIGIT_PINS[0], DIGIT_OFF);
  digitalWrite(DIGIT_PINS[1], DIGIT_OFF);
  setSegments(tens);
  digitalWrite(DIGIT_PINS[0], DIGIT_ON);
  delay(3);

  digitalWrite(DIGIT_PINS[0], DIGIT_OFF);
  setSegments(ones);
  digitalWrite(DIGIT_PINS[1], DIGIT_ON);
  delay(3);

  digitalWrite(DIGIT_PINS[1], DIGIT_OFF);
}

float readTemperatureC()
{
  const int raw = analogRead(TEMP_PIN);
  const float voltage = raw * (5.0f / 1023.0f);

  // LM35 output is 10 mV per degree Celsius.
  return voltage * 100.0f;
}

void updateTemperatureLED(float temperatureC)
{
  const unsigned long now = millis();

  if (now - lastBlinkTime >= BLINK_INTERVAL_MS) {
    lastBlinkTime = now;
    ledState = !ledState;
  }

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  if (temperatureC < 30.0f) {
    digitalWrite(GREEN_LED, ledState);
  } else if (temperatureC <= 40.0f) {
    digitalWrite(YELLOW_LED, ledState);
  } else {
    digitalWrite(RED_LED, ledState);
  }
}

void setup()
{
  Serial.begin(9600);

  for (uint8_t i = 0; i < 7; i++) {
    pinMode(SEGMENT_PINS[i], OUTPUT);
  }

  for (uint8_t i = 0; i < 2; i++) {
    pinMode(DIGIT_PINS[i], OUTPUT);
    digitalWrite(DIGIT_PINS[i], DIGIT_OFF);
  }

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
}

void loop()
{
  const unsigned long now = millis();

  if (now - lastTemperatureRead >= READ_INTERVAL_MS) {
    lastTemperatureRead = now;

    const float temperatureC = readTemperatureC();
    displayedTemperature = (int)(temperatureC + 0.5f);

    Serial.print(F("Temperature: "));
    Serial.print(temperatureC, 1);
    Serial.println(F(" C"));
  }

  updateTemperatureLED(readTemperatureC());

  // Repeatedly refresh the multiplexed display.
  showNumber(displayedTemperature);
}