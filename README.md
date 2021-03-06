# basilika
A soil moisture sensor version 1. Built on top of Nordic nRF5 SDK 15. 

# Setting up
## SDK 16.0
Download [Nordic SDK16.0](https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/) and install it at the project root.

## Toolchain
Segger Embedded Studio [SES](https://www.segger.com/products/development-tools/embedded-studio/) is recommended to use for developing. Start SES and open `basilika_pca10040.emProject` at the root level.

# Usage
Compile and flash the project to your board using SES.

## Hardware Components
### Analog Moisture Sensor
A relay moisture sensor bought from SparkFun [SEN-13322](https://www.sparkfun.com/products/13322) is used for measuring of moisture in soil.
Power Supply: 3.3V...5V
Output voltage signal: 0...4.2V (depends on provided voltage)
Consumption: 35mA

### NPN Power Transistor
BCP56T Series, 80V, 1A NPN medium power transistor, to control the moisture sensor.

## Enabled Drivers
### SAADC - Successive approximation analog-to-digital converter
8/10/12-bit resolution, 14-bit resolution with oversampling
Full scale input range (0 to VDD)
Each channel can be configured to select AIN0 to AIN7 pins, or the VDD pin.

### Clock
NRF legacy layer for a clock driver. 

### RTC
Real time counter driver.

## Pinout Configuration
|nRF52 DK|NPN Transistor|Moisture Sensor|
|--------|--------------|---------------|
|P0.30   |base		|		|
|P0.03	 |		|S		|
|GND	 |		|GND		|
|VDD	 |collector	|		|
|	 |emitter	|VCC		|

## Digital Output Formula
The formula is used to calculate the output voltage:
`result = [V(P) - V(N)] * gain/reference * 2^(resolution - m)`  
where
* **V(P)** : the voltage at input P
* **V(N)** : the voltage at input N
* **gain** : the selected gain
* **reference** : the selected reference voltage

# Repository Contents
* **/config** - Project configuration settings
* **/interface** - Analog and Digital Drivers
