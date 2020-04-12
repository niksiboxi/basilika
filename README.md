# basilika
Building a Soil Moisture Meter

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

