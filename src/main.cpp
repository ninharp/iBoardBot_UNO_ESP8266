#include <Arduino.h>
#include <stdint.h>

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

//A4988 StepperX(X_AXIS_STEPS_PER_UNIT, STEPPERX_DIR, STEPPERX_STEP, STEPPER_EN);
//A4988 StepperY(Y_AXIS_STEPS_PER_UNIT, STEPPERY_DIR, STEPPERY_STEP, STEPPER_EN);
//Servo Servo1;
//Servo Servo2;

wifiESP Wifi(false);

// Wifi status
bool Wconnected;
uint8_t Network_errors;
char MAC[13];  // MAC address of Wifi module

void setup()
{
    pinMode(WIFI_CONFIG_PIN, INPUT); // WIFI CONFIG BUTTON => sensor imput
    digitalWrite(WIFI_CONFIG_PIN, HIGH); // Enable Pullup on A0

    //StepperX.begin(RPM, MICROSTEPS);
    //StepperY.begin(RPM, MICROSTEPS);
    // if using enable/disable on ENABLE pin (active LOW) instead of SLEEP uncomment next line
    //StepperX.setEnableActiveState(LOW);
    //StepperY.setEnableActiveState(LOW);

    //Servo1.attach(9);  // attaches the servos on pin 9 and 10 to the servo object
    //Servo2.attach(10);

    Serial.begin(115200); // Serial output
    
    //delay(1000);

    // Manual Configuration mode? => Erase stored Wifi parameters (force A0 to ground)
    //if (digitalRead(WIFI_CONFIG_PIN) == LOW)
    //  Wifi.writeWifiConfigEEPROM(0, "", "", "", 0);
    //Wifi.writeWifiConfigEEPROM(0,"","","",0); // Force config

    
  #ifdef DEBUG
    delay(1000);  // Only needed for serial debug
    Serial.println(VERSION);
  #endif

    Serial.println(F("Reading Wifi configuration..."));
    //Wifi.readConfigEEPROM();

    Serial.println(Wifi.getStatus());
    Serial.println(Wifi.getSSID());
    //Serial.println(WifiConfig.pass);
    Serial.println("****");
    Serial.println(Wifi.getProxy());
    Serial.println(Wifi.getPort());

    // if wifi parameters are not configured we start the config web server
    if (Wifi.getStatus() != 1) {
        Wifi.wifiConfigurationMode();
    }

    Wconnected = false;
    Network_errors = 0;
    while (!Wconnected) {

        Wifi.reset();
        //Wifi.getMac(MAC); // Todo. its just read from the incoming stream but didnt initiate it

        if (Wifi.connect()) {
            //digitalWrite(13, HIGH); // enable servos
            Wconnected = true;
        }
        else {
            Serial.println(F("Error connecting to network!"));
            digitalWrite(13, LOW);
            delay(2500);
            Wconnected = false;
            Network_errors++;
            // If connection fail 2 times we enter WifiConfigurationMode
            if (Network_errors >= 2) {
              Wifi.wifiConfigurationMode();
              Network_errors = 0;
            }
        }
    }
}

void loop()
{
}