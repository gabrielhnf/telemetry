# BME280

Operating Range:

- Temperature: (-40, +85) °C, 
- Humidity: (0, 100)% relative humidity, accuracy of +- 3%,
- Pressure: (300, 1100) hPa

Accuracy:

- Temperature: +- 0.5 °C, 
- Humidity: +- 3% relative humidity,
- Pressure: +- 1.0 hPa

Resolution:

- Temperature: 0.01 °C, 
- Humidity: 0.008 % relative humidity,
- Pressure: 0.18 Pa

---

Objectives:

1. Support the full specified capabilities of the BME280 sensor.

2. Deliver measurements as quickly as the device and communication link supports, with little to no overhead in transmission and processing.

3. In the advent of an error or mismeasurement, recover if possible.

4. Support transmission of commands from the CLI to the STM32 in order to modify the sensor or device behavior.

5. Log events from the device or sensor in the CLI.

---

The default sensor configuration settings will follow the "Gaming" profile as described in the section 3.5.4 of the BME280 specification datasheet.
The CLI will be able to change each setting through the available commands.

Can log:

Bandwidth
Data output rate
Current consumption
Sensor mode
Oversampling settings
