/*
  ============================================================
              ARDUINO BASICS - WEEK #01
              Arduino Uno R3
  ============================================================

  QUESTION 1:
  Control an LED using two push buttons.

      Button 1 -> Turn LED ON
      Button 2 -> Turn LED OFF

  Hardware:
      ON button  = 100k pull-down resistor
      OFF button = 100k pull-up resistor

  NO software INPUT_PULLUP is used for Task 1.


  QUESTION 2:
  Traffic light using 3 LEDs.

      Green  -> 3 seconds
      Yellow -> 3 seconds
      Red    -> 3 seconds
      Repeat

  Hardware:
      Traffic button = 100k pull-up resistor

  BONUS:
  Pressing the button at any moment resets the traffic light
  back to GREEN and starts a new 3-second cycle.

  QUESTION 2 uses millis(), NOT delay().


  QUESTION 3:
  Traffic light using one RGB LED.

      Green  -> 3 seconds
      Yellow -> 3 seconds
      Red    -> 3 seconds
      Repeat

  A function with 3 parameters is used to control
  the RGB LED.


  ============================================================
*/


// ============================================================
// SELECT THE QUESTION
// ============================================================
//
// 1 = Task 1
// 2 = Task 2 + Bonus
// 3 = Task 3
//

#define CURRENT_QUESTION 2


// ============================================================
// QUESTION 1
// LED CONTROL USING TWO BUTTONS
// ============================================================

// LED connected to digital pin 8.
const byte QUESTION1_LED = 8;

// ON button connected to digital pin 2.
// Hardware pull-down resistor.
//
// Released = LOW
// Pressed  = HIGH
//
const byte BUTTON_ON = 2;

// OFF button connected to digital pin 3.
// Hardware pull-up resistor.
//
// Released = HIGH
// Pressed  = LOW
//
const byte BUTTON_OFF = 3;


// ------------------------------------------------------------
// Task 1 button states
// ------------------------------------------------------------

// Previous ON button state.
byte lastOnButtonState = LOW;

// Previous OFF button state.
byte lastOffButtonState = HIGH;


// ------------------------------------------------------------
// Task 1 debounce
// ------------------------------------------------------------

// Time when the ON button last changed.
unsigned long onLastChangeTime = 0;

// Time when the OFF button last changed.
unsigned long offLastChangeTime = 0;

// Debounce time.
const unsigned long debounceTime = 30;


// ============================================================
// QUESTION 2
// THREE-LED TRAFFIC LIGHT
// ============================================================

// Green LED connected to digital pin 5.
const byte GREEN_LED = 5;

// Yellow LED connected to digital pin 6.
const byte YELLOW_LED = 6;

// Red LED connected to digital pin 7.
const byte RED_LED = 7;

// Traffic-light reset button connected to digital pin 4.
const byte TRAFFIC_BUTTON = 4;


// ============================================================
// QUESTION 2
// TRAFFIC LIGHT STATES
// ============================================================

// 0 = Green
// 1 = Yellow
// 2 = Red
byte trafficState = 0;


// ============================================================
// QUESTION 2
// MILLIS TIMING
// ============================================================

// Time when the current traffic-light state started.
unsigned long previousMillis = 0;

// Each traffic-light color stays ON for 3 seconds.
const unsigned long trafficInterval = 3000;


// ------------------------------------------------------------
// Task 2 button states
// ------------------------------------------------------------

// Previous traffic button state.
//
// Pull-up resistor:
// Released = HIGH
//
byte lastTrafficButtonState = HIGH;

// Time when the traffic button last changed.
unsigned long trafficLastChangeTime = 0;


// ============================================================
// QUESTION 3
// RGB LED
// ============================================================

// RGB Red pin.
const byte RGB_RED = 9;

// RGB Green pin.
const byte RGB_GREEN = 10;

// RGB Blue pin.
const byte RGB_BLUE = 11;


// ============================================================
// QUESTION 3
// RGB COLOR VARIABLES
// ============================================================
//
// Common-cathode RGB LED:
//
// HIGH = ON
// LOW  = OFF
//

// GREEN
const byte GREEN_R = LOW;
const byte GREEN_G = HIGH;
const byte GREEN_B = LOW;

// YELLOW
const byte YELLOW_R = HIGH;
const byte YELLOW_G = HIGH;
const byte YELLOW_B = LOW;

// RED
const byte RED_R = HIGH;
const byte RED_G = LOW;
const byte RED_B = LOW;


// ============================================================
// SETUP
// ============================================================

void setup()
{
  // ==========================================================
  // QUESTION 1 SETUP
  // ==========================================================

  // Configure the LED as an output.
  pinMode(QUESTION1_LED, OUTPUT);

  // Configure ON button as a normal digital input.
  //
  // Hardware pull-down:
  // Released = LOW
  // Pressed  = HIGH
  //
  pinMode(BUTTON_ON, INPUT);

  // Configure OFF button as a normal digital input.
  //
  // Hardware pull-up:
  // Released = HIGH
  // Pressed  = LOW
  //
  pinMode(BUTTON_OFF, INPUT);


  // ==========================================================
  // QUESTION 2 SETUP
  // ==========================================================

  // Configure traffic LEDs as outputs.
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Configure traffic button as a normal input.
  //
  // NOTE:
  // Used INPUT_PULLUP
  // INPUT is correct.
  //
  pinMode(TRAFFIC_BUTTON, INPUT_PULLUP);


  // ==========================================================
  // QUESTION 3 SETUP
  // ==========================================================

  // Configure RGB pins as outputs.
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);


  // ==========================================================
  // INITIAL STATE
  // ==========================================================

  // Turn OFF Task 1 LED.
  digitalWrite(QUESTION1_LED, LOW);

  // Turn OFF traffic LEDs.
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  // Turn OFF RGB LED.
  setRGB(LOW, LOW, LOW);


  // ==========================================================
  // START QUESTION 2
  // ==========================================================

  if (CURRENT_QUESTION == 2)
  {
    // Start with GREEN.
    trafficState = 0;

    // Turn GREEN ON.
    digitalWrite(GREEN_LED, HIGH);

    // Start the traffic-light timer.
    previousMillis = millis();

    // Read the initial button state.
    lastTrafficButtonState = digitalRead(TRAFFIC_BUTTON);
  }
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
  // Run Task 1.
  if (CURRENT_QUESTION == 1)
  {
    task1();
  }

  // Run Task 2.
  else if (CURRENT_QUESTION == 2)
  {
    task2();
  }

  // Run Task 3.
  else if (CURRENT_QUESTION == 3)
  {
    task3();
  }
}


// ============================================================
// QUESTION 1
// TWO BUTTON LED CONTROL
// ============================================================

void task1()
{
  // Read the ON button.
  byte onButtonState = digitalRead(BUTTON_ON);

  // Read the OFF button.
  byte offButtonState = digitalRead(BUTTON_OFF);


  // ==========================================================
  // ON BUTTON
  // ==========================================================

  // Detect a change in the ON button.
  if (onButtonState != lastOnButtonState)
  {
    // Restart debounce timer.
    onLastChangeTime = millis();

    // Save the new state.
    lastOnButtonState = onButtonState;
  }


  // Check whether the ON button has remained
  // stable for the debounce period.
  if (millis() - onLastChangeTime >= debounceTime)
  {
    // ON button is pressed when state is HIGH.
    if (onButtonState == HIGH)
    {
      // Turn ON the LED.
      digitalWrite(QUESTION1_LED, HIGH);
    }
  }


  // ==========================================================
  // OFF BUTTON
  // ==========================================================

  // Detect a change in the OFF button.
  if (offButtonState != lastOffButtonState)
  {
    // Restart debounce timer.
    offLastChangeTime = millis();

    // Save the new state.
    lastOffButtonState = offButtonState;
  }


  // Check whether the OFF button has remained
  // stable for the debounce period.
  if (millis() - offLastChangeTime >= debounceTime)
  {
    // OFF button is pressed when state is LOW.
    if (offButtonState == LOW)
    {
      // Turn OFF the LED.
      digitalWrite(QUESTION1_LED, LOW);
    }
  }
}


// ============================================================
// QUESTION 2
// TRAFFIC LIGHT + RESET BUTTON
// ============================================================

void task2()
{
  // ----------------------------------------------------------
  // CHECK THE BUTTON
  // ----------------------------------------------------------

  // Read the current button state.
  byte trafficButtonState = digitalRead(TRAFFIC_BUTTON);


  // ----------------------------------------------------------
  // BUTTON DEBOUNCE
  // ----------------------------------------------------------

  // Detect a change in the button.
  if (trafficButtonState != lastTrafficButtonState)
  {
    // Restart the debounce timer.
    trafficLastChangeTime = millis();

    // Save the new button state.
    lastTrafficButtonState = trafficButtonState;
  }


  // Check whether the button has been stable
  // for the debounce period.
  if (millis() - trafficLastChangeTime >= debounceTime)
  {
    // Because we use a pull-up:
    //
    // HIGH = released
    // LOW  = pressed
    //
    // A LOW state means the button is pressed.

    if (trafficButtonState == LOW)
    {
      // Reset the traffic light.
      resetTrafficLight();

      // Wait for button release logically.
      //
      // We don't use delay().
      //
      // Resetting lastTrafficButtonState prevents
      // repeated resets caused by the same press.
      lastTrafficButtonState = LOW;
    }
  }


  // ----------------------------------------------------------
  // TRAFFIC LIGHT TIMER
  // ----------------------------------------------------------

  // Get current time.
  unsigned long currentMillis = millis();


  // Check whether 3 seconds have passed.
  if (currentMillis - previousMillis >= trafficInterval)
  {
    // Restart the timer.
    previousMillis = currentMillis;


    // Move to the next state.
    trafficState++;


    // After RED, return to GREEN.
    if (trafficState > 2)
    {
      trafficState = 0;
    }


    // Update the LEDs.
    updateTrafficLight();
  }
}


// ============================================================
// QUESTION 2
// RESET TRAFFIC LIGHT
// ============================================================

void resetTrafficLight()
{
  // Turn OFF all traffic LEDs.
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);


  // Set state to GREEN.
  trafficState = 0;


  // Turn GREEN ON.
  digitalWrite(GREEN_LED, HIGH);


  // Restart the 3-second timer.
  previousMillis = millis();
}


// ============================================================
// QUESTION 2
// UPDATE TRAFFIC LIGHT
// ============================================================

void updateTrafficLight()
{
  // Turn OFF all LEDs first.
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);


  // GREEN
  if (trafficState == 0)
  {
    digitalWrite(GREEN_LED, HIGH);
  }


  // YELLOW
  else if (trafficState == 1)
  {
    digitalWrite(YELLOW_LED, HIGH);
  }


  // RED
  else if (trafficState == 2)
  {
    digitalWrite(RED_LED, HIGH);
  }
}


// ============================================================
// QUESTION 3
// RGB TRAFFIC LIGHT
// ============================================================

void task3()
{
  // GREEN
  setRGB(GREEN_R, GREEN_G, GREEN_B);

  // Wait 3 seconds.
  delay(3000);


  // YELLOW
  setRGB(YELLOW_R, YELLOW_G, YELLOW_B);

  // Wait 3 seconds.
  delay(3000);


  // RED
  setRGB(RED_R, RED_G, RED_B);

  // Wait 3 seconds.
  delay(3000);


  // Turn OFF RGB LED.
  setRGB(LOW, LOW, LOW);
}


// ============================================================
// QUESTION 3
// RGB LED CONTROL FUNCTION
// ============================================================
//
// This function has exactly 3 parameters:
//
// redValue
// greenValue
// blueValue
//

void setRGB(byte redValue, byte greenValue, byte blueValue)
{
  // Control RED.
  digitalWrite(RGB_RED, redValue);

  // Control GREEN.
  digitalWrite(RGB_GREEN, greenValue);

  // Control BLUE.
  digitalWrite(RGB_BLUE, blueValue);
}