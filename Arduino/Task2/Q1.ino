
/*
  Week 02 - Task 1
  Control LED brightness using a potentiometer.

  Potentiometer: 5V, GND, and middle pin to A0.
  LED: D9 -> resistor -> LED anode; LED cathode -> GND.
*/

const uint8_t POT_PIN = A0;
const uint8_t LED_PIN = 9; // PWM-capable pin on Uno/Nano

void setup()
{
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  const int analogValue = analogRead(POT_PIN); // 0..1023

  // Scale the ADC range to the PWM range.
  int brightness = map(analogValue, 0, 1023, 0, 255);

  // Keep the PWM value in its legal range.
  brightness = constrain(brightness, 0, 255);

  analogWrite(LED_PIN, brightness);

  Serial.print(F("Analog: "));
  Serial.print(analogValue);
  Serial.print(F(" | Brightness: "));
  Serial.println(brightness);

  delay(10);
}