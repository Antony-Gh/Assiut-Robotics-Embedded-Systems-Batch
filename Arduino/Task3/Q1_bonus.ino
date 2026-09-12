
/*
  Week 3 - Task 1 Bonus
  Motor speed increases with measured distance.

  The proportional mapping is:
    distance <= STOP_DISTANCE  -> speed = 0
    distance >= MAX_DISTANCE   -> speed = MAX_SPEED
    otherwise                  -> speed rises linearly

  This is an open-loop speed command, not closed-loop RPM control.
*/

const uint8_t EN_LEFT = 5;
const uint8_t LEFT_IN1 = 7;
const uint8_t LEFT_IN2 = 8;

const uint8_t EN_RIGHT = 6;
const uint8_t RIGHT_IN1 = 9;
const uint8_t RIGHT_IN2 = 10;

const uint8_t TRIG_PIN = 12;
const uint8_t ECHO_PIN = 13;

const float STOP_DISTANCE_CM = 10.0;
const float MAX_DISTANCE_CM = 100.0;
const uint8_t MAX_SPEED = 230;

float readDistanceCm()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Timeout prevents pulseIn() from blocking for too long.
  const unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000UL);

  if (duration == 0) {
    return -1.0; // No echo / out of range
  }

  return duration * 0.0343f / 2.0f;
}

uint8_t distanceToSpeed(float distanceCm)
{
  if (distanceCm <= STOP_DISTANCE_CM) {
    return 0;
  }

  if (distanceCm >= MAX_DISTANCE_CM) {
    return MAX_SPEED;
  }

  const float fraction =
    (distanceCm - STOP_DISTANCE_CM) /
    (MAX_DISTANCE_CM - STOP_DISTANCE_CM);

  return (uint8_t)(fraction * MAX_SPEED);
}

void setForward(uint8_t speed)
{
  digitalWrite(LEFT_IN1, HIGH);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, HIGH);
  digitalWrite(RIGHT_IN2, LOW);

  analogWrite(EN_LEFT, speed);
  analogWrite(EN_RIGHT, speed);
}

void stopMotors()
{
  analogWrite(EN_LEFT, 0);
  analogWrite(EN_RIGHT, 0);

  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, LOW);
}

void setup()
{
  Serial.begin(9600);

  pinMode(EN_LEFT, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);

  pinMode(EN_RIGHT, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  stopMotors();
}

void loop()
{
  const float distanceCm = readDistanceCm();

  // Fail-safe: stop if the sensor returns no echo.
  if (distanceCm < 0) {
    stopMotors();
    Serial.println(F("No echo: motors stopped"));
    delay(100);
    return;
  }

  const uint8_t speed = distanceToSpeed(distanceCm);

  if (speed == 0) {
    stopMotors();
  } else {
    setForward(speed);
  }

  Serial.print(F("Distance: "));
  Serial.print(distanceCm);
  Serial.print(F(" cm | PWM: "));
  Serial.println(speed);

  delay(100);
}