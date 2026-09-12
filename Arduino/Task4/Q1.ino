
#include <IRremote.hpp>

// ---------- Pins ----------
const uint8_t IR_PIN = 2;
const uint8_t ENA_PIN = 5;  // Must be a PWM pin
const uint8_t IN1_PIN = 7;
const uint8_t IN2_PIN = 8;

// ---------- Replace these with your remote's command values ----------
const uint8_t CMD_FORWARD = 0x18;
const uint8_t CMD_REVERSE = 0x52;
const uint8_t CMD_STOP    = 0x1C;
const uint8_t CMD_SPEED_UP   = 0x5A;
const uint8_t CMD_SPEED_DOWN = 0x08;

// ---------- Motor state ----------
int motorSpeed = 150;       // PWM range: 0..255
const int SPEED_STEP = 25;
const int MIN_SPEED = 0;
const int MAX_SPEED = 255;

enum MotorDirection {
  MOTOR_STOPPED,
  MOTOR_FORWARD,
  MOTOR_REVERSE
};

MotorDirection direction = MOTOR_STOPPED;

void applyMotor()
{
  if (direction == MOTOR_FORWARD) {
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
  } else if (direction == MOTOR_REVERSE) {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
  } else {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
  }

  analogWrite(ENA_PIN, direction == MOTOR_STOPPED ? 0 : motorSpeed);
}

void setup()
{
  Serial.begin(9600);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  applyMotor();

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("IR motor control ready."));
  Serial.println(F("Press remote buttons; note the printed command values."));
}

void loop()
{
  if (!IrReceiver.decode()) {
    return;
  }

  // Print the command so you can identify your remote's buttons.
  Serial.print(F("IR command: 0x"));
  Serial.println(IrReceiver.decodedIRData.command, HEX);

  // Ignore repeat frames to avoid repeated actions when holding a button.
  if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
    const uint8_t command = IrReceiver.decodedIRData.command;

    switch (command) {
      case CMD_FORWARD:
        direction = MOTOR_FORWARD;
        break;

      case CMD_REVERSE:
        direction = MOTOR_REVERSE;
        break;

      case CMD_STOP:
        direction = MOTOR_STOPPED;
        break;

      case CMD_SPEED_UP:
        motorSpeed = min(motorSpeed + SPEED_STEP, MAX_SPEED);
        break;

      case CMD_SPEED_DOWN:
        motorSpeed = max(motorSpeed - SPEED_STEP, MIN_SPEED);
        if (motorSpeed == 0) {
          direction = MOTOR_STOPPED;
        }
        break;

      default:
        // Unknown button: do nothing.
        break;
    }

    applyMotor();

    Serial.print(F("Speed: "));
    Serial.print(motorSpeed);
    Serial.print(F(" | Direction: "));
    Serial.println(direction == MOTOR_FORWARD ? F("Forward") :
                   direction == MOTOR_REVERSE ? F("Reverse") : F("Stopped"));
  }

  IrReceiver.resume();
}