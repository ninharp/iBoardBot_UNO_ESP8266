#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_
// iBoardBot Arduino UNO project configuration file
#include <Arduino.h>

// Pin configuration
#define STEPPER_EN        8
#define STEPPERX_STEP     2
#define STEPPERX_DIR      5
#define STEPPERY_STEP     4
#define STEPPERY_DIR      7
#define SERVO1_PIN        9
#define SERVO2_PIN        10
#define WIFI_CONFIG_PIN   A0

#define DEBUG 1

#define VERSION "iBoardBot Arduino UNO with CNC Shield v1.0.0.1"

// This depends on the pulley teeth and microstepping. For 20 teeth GT2 and 1/16 => 80  200*16 = 3200/(20*2mm) = 80
#define X_AXIS_STEPS_PER_UNIT 80    
// This depends on the pulley teeth and microstepping. For 42 teeth GT2 and 1/16 => 38  200*16 = 3200/(42*2mm) = 38
#define Y_AXIS_STEPS_PER_UNIT 38 

// Servo values por NoPaint, Paint and Erase servo positions...
#define SERVO1_PAINT 180
#define SERVO1_ERASER 105
#define SERVO2_LIFT 20
#define SERVO2_PAINT 120

#endif