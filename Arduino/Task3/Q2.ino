
/*
  Week 3 - Task 2
  Password-protected digital lock using:
  - 4x4 keypad
  - 16x2 I2C LCD
  - Servo motor
  - Buzzer

  Keypad:
    '#' = submit password
    '*' = clear typed password

  Correct password:
    Servo moves to 90 degrees.

  Wrong password:
    Servo moves to 0 degrees and buzzer sounds for 3 seconds.

  The LCD displays asterisks instead of the actual password.
*/

#include <Keypad.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Hardware ----------
const uint8_t SERVO_PIN = 10;
const uint8_t BUZZER_PIN = 11;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo lockServo;

// ---------- Keypad ----------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------- Password settings ----------
const char CORRECT_PASSWORD[] = "1234";
const uint8_t PASSWORD_LENGTH = sizeof(CORRECT_PASSWORD) - 1;

// Buffer includes space for the null terminator.
char enteredPassword[PASSWORD_LENGTH + 1];
uint8_t enteredLength = 0;

// ---------- Alarm timing ----------
const unsigned long ALARM_DURATION_MS = 3000;
unsigned long alarmStartTime = 0;
bool alarmActive = false;

// ---------- LCD helpers ----------
void showEntryScreen()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Enter password:"));

  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < enteredLength; i++) {
    lcd.print('*');
  }
}

void clearPassword()
{
  enteredLength = 0;
  enteredPassword[0] = '\0';
  showEntryScreen();
}

void startAlarm()
{
  digitalWrite(BUZZER_PIN, HIGH);
  alarmStartTime = millis();
  alarmActive = true;
}

void stopAlarmIfFinished()
{
  if (alarmActive && millis() - alarmStartTime >= ALARM_DURATION_MS) {
    digitalWrite(BUZZER_PIN, LOW);
    alarmActive = false;
  }
}

void checkPassword()
{
  enteredPassword[enteredLength] = '\0';

  if (strcmp(enteredPassword, CORRECT_PASSWORD) == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Access granted"));

    lockServo.write(90);
    delay(1500);

    // Keep the lock open until the next attempt.
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Unlocked"));
    lcd.setCursor(0, 1);
    lcd.print(F("Press * to reset"));
  } else {
    lockServo.write(0);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Wrong password"));

    lcd.setCursor(0, 1);
    lcd.print(F("Alarm: 3 seconds"));

    startAlarm();
  }

  // Clear the typed password after either result.
  enteredLength = 0;
  enteredPassword[0] = '\0';
}

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  lockServo.attach(SERVO_PIN);
  lockServo.write(0);

  lcd.init();
  lcd.backlight();

  clearPassword();
}

void loop()
{
  stopAlarmIfFinished();

  const char key = keypad.getKey();

  if (key == NO_KEY) {
    return;
  }

  // While the wrong-password alarm is active, ignore keypad input.
  if (alarmActive) {
    return;
  }

  if (key == '*') {
    clearPassword();
    return;
  }

  if (key == '#') {
    if (enteredLength > 0) {
      checkPassword();
    }
    return;
  }

  // Accept only numeric password characters.
  if (key >= '0' && key <= '9' && enteredLength < PASSWORD_LENGTH) {
    enteredPassword[enteredLength] = key;
    enteredLength++;
    enteredPassword[enteredLength] = '\0';

    // Show only stars, never the entered digits.
    lcd.setCursor(0, 1);
    for (uint8_t i = 0; i < enteredLength; i++) {
      lcd.print('*');
    }
    for (uint8_t i = enteredLength; i < 16; i++) {
      lcd.print(' ');
    }
  }
}