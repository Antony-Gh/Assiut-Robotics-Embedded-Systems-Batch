
/*
  Week 3 - Task 1
  Two DC motors controlled by four pushbuttons through an L293D.

  Button wiring: Arduino pin to button to GND.
  INPUT_PULLUP means pressed = LOW.

  Movement:
  - Forward: both motors forward
  - Backward: both motors backward
  - Turn right: left motor forward, right motor backward
  - Turn left: left motor backward, right motor forward
  - No button: stop
*/

const uint8_t EN_LEFT = 5;   // PWM pin
const uint8_t LEFT_IN1 = 7;
const uint8_t LEFT_IN2 = 8;

const uint8_t EN_RIGHT = 6;  // PWM pin
const uint8_t RIGHT_IN1 = 9;
const uint8_t RIGHT_IN2 = 10;

const uint8_t BTN_FORWARD  = 2;
const uint8_t BTN_BACKWARD = 3;
const uint8_t BTN_RIGHT    = 4;
const uint8_t BTN_LEFT     = 11;

const uint8_t MOTOR_SPEED = 200; // 0..255

void setLeftMotor(int8_t direction, uint8_t speed)
{
  // direction: 1 = forward, -1 = backward, 0 = stop
  if (direction > 0) {
    digitalWrite(LEFT_IN1, HIGH);
    digitalWrite(LEFT_IN2, LOW);
  } else if (direction < 0) {
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, HIGH);
  } else {
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, LOW);
    speed = 0;
  }

  analogWrite(EN_LEFT, speed);
}

void setRightMotor(int8_t direction, uint8_t speed)
{
  if (direction > 0) {
    digitalWrite(RIGHT_IN1, HIGH);
    digitalWrite(RIGHT_IN2, LOW);
  } else if (direction < 0) {
    digitalWrite(RIGHT_IN1, LOW);
    digitalWrite(RIGHT_IN2, HIGH);
  } else {
    digitalWrite(RIGHT_IN1, LOW);
    digitalWrite(RIGHT_IN2, LOW);
    speed = 0;
  }

  analogWrite(EN_RIGHT, speed);
}

void stopMotors()
{
  setLeftMotor(0, 0);
  setRightMotor(0, 0);
}

void forward()
{
  setLeftMotor(1, MOTOR_SPEED);
  setRightMotor(1, MOTOR_SPEED);
}

void backward()
{
  setLeftMotor(-1, MOTOR_SPEED);
  setRightMotor(-1, MOTOR_SPEED);
}

void turn_right()
{
  setLeftMotor(1, MOTOR_SPEED);
  setRightMotor(-1, MOTOR_SPEED);
}

void turn_left()
{
  setLeftMotor(-1, MOTOR_SPEED);
  setRightMotor(1, MOTOR_SPEED);
}

void setup()
{
  pinMode(EN_LEFT, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);

  pinMode(EN_RIGHT, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);

  pinMode(BTN_FORWARD, INPUT_PULLUP);
  pinMode(BTN_BACKWARD, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);

  stopMotors();
}

void loop()
{
  // Priority prevents conflicting movement commands if multiple
  // buttons are pressed at the same time.
  if (digitalRead(BTN_FORWARD) == LOW) {
    forward();
  } else if (digitalRead(BTN_BACKWARD) == LOW) {
    backward();
  } else if (digitalRead(BTN_RIGHT) == LOW) {
    turn_right();
  } else if (digitalRead(BTN_LEFT) == LOW) {
    turn_left();
  } else {
    stopMotors();
  }
}