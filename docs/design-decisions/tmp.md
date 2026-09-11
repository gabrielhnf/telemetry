Objectives:

1. Support the full specified capabilities of the BME280 sensor.

2. Deliver measurements as quickly as the device and communication link supports, with little to no overhead in transmission and processing.

3. In the advent of an error or mismeasurement, recover if possible.

4. Support transmission of commands from the CLI to the STM32 in order to modify the sensor or device behavior.

5. Log events from the device or sensor in the CLI.

---

# Problem

A specific number format must be chosen to correctly represent meaningful information transmitted through the communication link.

# Options considered

1. ASCII
2. IEEE-754 floating point
3. Fixed point

---

## ASCII

### Upsides

Human-readable.
A serial monitor can be used to have a manual look on the value being transmitted.
Easier to debug.

### Downsides

The payload size can be length-variable.

The sending sensor would have the need to convert the number to a string of characters,
and the receiving device would have the need to convert the string of characters to a proper number,
making the processing phase longer.

## IEEE-754

### Upsides

Standard for modern hardware use.

Constant payload size.

Can represent a much wider range of numbers than the ASCII representation.
Has superior granularity to the fixed point representation.

Processing almost non-existent.

### Downsides

Accuracy is not guaranteed for every measurement.

Precision is not uniform.

Special values such as `+0, -0, NaN, +inf, -inf` are invalid.

## Fixed point representation

### Upsides

The BME280 sensor output compensation returns fixed-point measurements as per specification recommendation and API usage.

The sensor device precision can be uniformly and exactly represented.

### Downsides

It has a narrower range of number representation in relation to the IEEE-754.
