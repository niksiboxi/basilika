#ifndef APPLICATION_CONFIG_H
#define APPLICATION_CONFIG_H

// SAADC Configuration
#define SAMPLES_IN_BUFFER 1   // Size of SAADC buffer
#define SAADC_MVOLTS_MULTIPLIER  71  // For conversion ADC into mV

// RTC Configuration
#define COMPARE_COUNTERTIME (1UL) // Compare event timeout (sec.)

// GPIO Configuration
#define TBASE_SET_PIN 30 // Set gpio port P0.30 to control NPN Transistor Base

#define TWI_INSTANCE  0
#define TWI_SCL_PIN   27
#define TWI_SDA_PIN   26

#endif