#ifndef APPLICATION_CONFIG_H
#define APPLICATION_CONFIG_H

// SAADC Configuration
#define SAMPLES_IN_BUFFER 1 // Size of SAADC buffer

// RTC Configuration
#define COMPARE_COUNTERTIME (1UL) // Compare event timeout (sec.)

// GPIO Configuration
#define TBASE_SET_PIN 30 // Set gpio port P0.30 to control NPN Transistor Base
#define TFT_LITE      9  // TFT Display backlight control pin

#endif