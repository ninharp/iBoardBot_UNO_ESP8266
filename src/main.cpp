#include <Arduino.h>
#include <stdint.h>
// Needed libraries
#include <SoftwareSerial.h>
#include <BasicStepperDriver.h>
#include <A4988.h>
#include <Servo.h>
#include <wifiESP.h>

// Configuration
#include <configuration.h>

#define RPM 20
// Since microstepping is set externally, make sure this matches the selected mode
// If it doesn't, the motor will move at a different RPM than chosen
// 1=full step, 2=half step etc.
#define MICROSTEPS 16

A4988 StepperX(X_AXIS_STEPS_PER_UNIT, STEPPERX_DIR, STEPPERX_STEP, STEPPER_EN);
A4988 StepperY(Y_AXIS_STEPS_PER_UNIT, STEPPERY_DIR, STEPPERY_STEP, STEPPER_EN);
Servo Servo1;
Servo Servo2;
SoftwareSerial DebugSerial(A4, A5);

void WifiConfigurationMode()
{

}

void setup()
{
  pinMode(WIFI_CONFIG_PIN, INPUT); // WIFI CONFIG BUTTON => sensor imput
  digitalWrite(WIFI_CONFIG_PIN, HIGH); // Enable Pullup on A0

  StepperX.begin(RPM, MICROSTEPS);
  StepperY.begin(RPM, MICROSTEPS);
  // if using enable/disable on ENABLE pin (active LOW) instead of SLEEP uncomment next line
  StepperX.setEnableActiveState(LOW);
  StepperY.setEnableActiveState(LOW);

  Servo1.attach(9);  // attaches the servos on pin 9 and 10 to the servo object
  Servo2.attach(10);

  Serial.begin(115200); // Serial output for wifi module
  DebugSerial.begin(57600); // DebugSerial output to console
  
  delay(1000);

  // Manual Configuration mode? => Erase stored Wifi parameters (force A0 to ground)
  //if (digitalRead(WIFI_CONFIG_PIN) == LOW)
  //  writeWifiConfig(0, "", "", "", 0);
  //writeWifiConfig(0,"","","",0); // Force config

#ifdef DEBUG
  //delay(10000);  // Only needed for serial debug
  DebugSerial.println(VERSION);
#endif

  DebugSerial.println(F("Reading Wifi configuration..."));

}

void loop()
{
}