# basilika
A soil moisture sensor version 1. Built on top of Nordic nRF5 SDK 15. 

# Setting up
## SDK 15.3
Download [Nordic SDK15.3](https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/) and install it at the project root.

## Toolchain
Segger Embedded Studio (SES) is recommended to use for developing. Start SES and open 'basilika_pca10040.emProject' at root level.

# Usage
Compile and flash the project to your board using SES.

# Analog Soil Mosture Sensor
Power Supply: 3.3v or 5V
Output voltage singal: 0~4.2V
Current: 35mA

# SAADC - Successive approximation analog-to-digital converter
8/10/12-bit resolution, 14-bit resolution with oversampling
Full scale input range (0 to VDD)
Each channel can be configured to select AIN0 to AIN7 pins, or the VDD pin.

# Digital Ouput
RESULT = [V(P) - V(N)] * GAIN/REFERENCE * 2^(RESOLUTION - m)
V(P) : the voltage at input P
V(N) : the voltage at input N
GAIN : the selected gain setting
REFERENCE : the selected reference voltage

