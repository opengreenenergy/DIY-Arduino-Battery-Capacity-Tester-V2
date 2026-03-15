/*
====================================================================================================================
  PROJECT       : DIY Arduino Battery Capacity Tester
  VERSION       : V2.0
  DATE          : 09-Nov-2019
  AUTHOR        : Open Green Energy (Deba168, India)

  ORIGINAL REFERENCE
  ------------------------------------------------------------------------------------------------------------------
  Based on battery capacity measurement concept shared by Hesam Moshiri
  https://www.pcbway.com/blog/technology/Battery_capacity_measurement_using_Arduino.html

  LICENSE
  ------------------------------------------------------------------------------------------------------------------
  Copyright (c) 2026 Open Green Energy

  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-nc-sa/4.0/

  HARDWARE USED
  ------------------------------------------------------------------------------------------------------------------
  MCU            : Arduino Nano / Arduino Uno
  DISPLAY        : OLED 128x64 SSD1306 (I2C)
  BUTTONS        : UP and DOWN push buttons
  LOAD CONTROL   : PWM controlled MOSFET electronic load
  SENSING        : Battery voltage measurement on A0
  BUZZER         : Status buzzer

  WHAT THIS DEVICE DOES
  ------------------------------------------------------------------------------------------------------------------
  1. User selects discharge current using UP and DOWN buttons.
  2. Battery is discharged through a PWM controlled load.
  3. Battery voltage is continuously measured.
  4. Time elapsed is tracked in hours, minutes and seconds.
  5. Capacity is calculated using current × time.
  6. OLED displays time, voltage, discharge current and calculated capacity.
  7. Test automatically stops when battery voltage reaches cutoff level.

====================================================================================================================
*/


#include <JC_Button.h>
#include <Adafruit_SSD1306.h>


// ============================================================================
// OLED DISPLAY CONFIGURATION
// ============================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// ============================================================================
// BATTERY TEST PARAMETERS
// ============================================================================
const float Low_BAT_level = 3.0;      // Cutoff voltage

// Current steps with a 3Ω load resistor (R7)
const int Current [] = {0,110,210,300,390,490,580,680,770,870,960,1000};


// ============================================================================
// PIN DEFINITIONS
// ============================================================================
const byte PWM_Pin = 10;   // PWM output controlling discharge MOSFET
const byte Buzzer  = 9;    // Buzzer output
const int BAT_Pin  = A0;   // Battery voltage measurement pin


// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
int PWM_Value = 0;                 // PWM duty cycle controlling discharge current
unsigned long Capacity = 0;        // Calculated battery capacity in mAh
int ADC_Value = 0;

float Vcc = 4.96;                  // Arduino supply voltage (measured manually)
float BAT_Voltage = 0;             // Battery voltage
float sample = 0;                  // ADC averaging variable

byte Hour = 0, Minute = 0, Second = 0;

bool calc = false;                 // True when test starts
bool Done = false;                 // True when test finishes


// ============================================================================
// BUTTON OBJECTS
// ============================================================================
Button UP_Button(2, 25, false, true);
Button Down_Button(3, 25, false, true);


// ============================================================================
// SETUP FUNCTION
// Runs once at power-up
// ============================================================================
void setup () {

  Serial.begin(9600);

  pinMode(PWM_Pin, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  analogWrite(PWM_Pin, PWM_Value);

  UP_Button.begin();
  Down_Button.begin();

  // Initialize OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Startup screen
  display.setTextSize(1);
  display.setCursor(12,25);
  display.print("Open Green Energy");
  display.display();
  delay(3000);

  // Current adjustment screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(2,15);
  display.print("Adj Curr:");
  display.setCursor(2,40);
  display.print("UP/Down:");
  display.print("0");
  display.display();
}


// ============================================================================
// MAIN LOOP
// Handles button input and starts the discharge test
// ============================================================================
void loop() {

  UP_Button.read();
  Down_Button.read();

  // Increase discharge current
  if (UP_Button.wasReleased() && PWM_Value < 55 && calc == false)
  {
    PWM_Value = PWM_Value + 5;
    analogWrite(PWM_Pin, PWM_Value);

    display.clearDisplay();
    display.setCursor(2,25);
    display.print("Curr:");
    display.print(String(Current[PWM_Value / 5]) + "mA");
    display.display();
  }

  // Decrease discharge current
  if (Down_Button.wasReleased() && PWM_Value > 1 && calc == false)
  {
    PWM_Value = PWM_Value - 5;
    analogWrite(PWM_Pin, PWM_Value);

    display.clearDisplay();
    display.setCursor(2,25);
    display.print("Curr:");
    display.print(String(Current[PWM_Value / 5]) + "mA");
    display.display();
  }

  // Start test when UP button is held for 1 second
  if (UP_Button.pressedFor(1000) && calc == false)
  {
    digitalWrite(Buzzer, HIGH);
    delay(100);
    digitalWrite(Buzzer, LOW);

    display.clearDisplay();
    timerInterrupt();
  }
}


// ============================================================================
// BATTERY DISCHARGE TEST ROUTINE
// Calculates battery capacity while discharging
// ============================================================================
void timerInterrupt(){

  calc = true;

  while (Done == false)
  {
    // Time calculation
    Second++;

    if (Second == 60)
    {
      Second = 0;
      Minute++;
    }

    if (Minute == 60)
    {
      Minute = 0;
      Hour++;
    }

    // ------------------------------------------------------------------------
    // Measure battery voltage (average of 100 samples)
    // ------------------------------------------------------------------------
    for(int i=0;i<100;i++)
    {
      sample = sample + analogRead(BAT_Pin);
      delay(2);
    }

    sample = sample / 100;
    BAT_Voltage = sample * Vcc / 1024.0;


    // ------------------------------------------------------------------------
    // Display real-time parameters
    // ------------------------------------------------------------------------
    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(20,5);
    display.print(String(Hour) + ":" + String(Minute) + ":" + String(Second));

    display.setTextSize(1);
    display.setCursor(0,25);
    display.print("Disch Curr: ");
    display.print(String(Current[PWM_Value / 5]) + "mA");

    display.setCursor(2,40);
    display.print("Bat Volt:" + String(BAT_Voltage) + "V");


    // Capacity calculation
    Capacity = (Hour * 3600) + (Minute * 60) + Second;
    Capacity = (Capacity * Current[PWM_Value / 5]) / 3600;

    display.setCursor(2,55);
    display.print("Capacity:" + String(Capacity) + "mAh");

    display.display();


    // ------------------------------------------------------------------------
    // Stop test when battery voltage reaches cutoff level
    // ------------------------------------------------------------------------
    if (BAT_Voltage < Low_BAT_level)
    {
      Capacity = (Hour * 3600) + (Minute * 60) + Second;
      Capacity = (Capacity * Current[PWM_Value / 5]) / 3600;

      display.clearDisplay();
      display.setTextSize(2);

      display.setCursor(2,15);
      display.print("Capacity:");

      display.setCursor(2,40);
      display.print(String(Capacity) + "mAh");

      display.display();

      Done = true;

      PWM_Value = 0;
      analogWrite(PWM_Pin, PWM_Value);

      // Completion buzzer indication
      digitalWrite(Buzzer, HIGH);
      delay(100);
      digitalWrite(Buzzer, LOW);
      delay(100);
      digitalWrite(Buzzer, HIGH);
      delay(100);
      digitalWrite(Buzzer, LOW);
      delay(100);
    }

    delay(1000);
  }
}
