# Problem

A specific number format must be chosen to correctly represent meaningful information.
The data that comes from and to the STM32 or PC must be standardized in such a way that information is not lost or wrongfully used/interpreted.

# Options considered

1. ASCII
2. IEEE-754 floating point
3. Fixed point

# Decision

It has been decided to use the fixed point representation, due to the forgiving nature of the normal conditions where the project is applicable, both in range and in accuracy, for both temperature and pressure.

# Why

Normal conditions assume a small range of temperature and pressure change, with non-urgency for inaccuracies in readings.

# Downsides

Aside from the downsides specified in the "Specifics" section, it is currently unknown on how to proceed with the reading of timestamp, which is a integer number that needs to support long values.

# What would make me reconsider

The need for higher accuracy or support of abnormal readings, such as high temperature and/or pressure

# Specifics

## ASCII

### What it is

ASCII number representation in serial communication sends each digit of a number as a separate text character instead of a raw binary value.

### Upsides

It is significantly more human-readable.
A serial monitor can be used to have a manual look on what it being sent.
It can also use its set of control characters to represent things such as the start and end of a message, etc.

It is used in some commercial devices, such as GPS NMEA.
> Common GPS NMEA message:
> $GPGGA,181908.00,3404.7041778,N,07044.3966270,W,4,13,1.00,495.144,M,29.200,M,0.10,0000*40

Since it's easier to read and implement in relation to other formats, it's easier to have and write better software support for the device.

### Downsides

Representing data such as numbers in ASCII tend to occupy much more space than raw binary.
Say, for example, one needs to send temperature data from one device to another using ASCII.

> temperature = 23.7 C

Disregarding field identification and measurement unit.
The useful data is `23.7` only.

Each character in ASCII takes one byte (8 bits).
That means that for a device to receive the reading, it must read a total of 4 bytes or 32 bits, counting the `.` decimal separator.
This is an expressive waste of space when taking into account other formats, which usually uses fewer bytes to represent not only one digit but the entire value (such as the float representation, commonly taking 4 bytes of memory).

Not only that, but since every character must be sent separately, each character packet would need it's own protocol headers, sequence numbers, error correction code, etc. And a loss of packet would not only invalidate the lost packet, but also those that came before it if the loss happens in the middle of a reading (resulting in even more data needing to be sent).

The sparsely transmitted data would also reduce the practical sample rate of the device, since it would need more than one sample to represent one singular reading. This way, a 1Hz transmission rate might not exactly be equivalent to a 1Hz sample rate.

The receiving end also has the need to convert the string of characters to a proper number again, which takes an extra step.

## IEEE-754

### What it is

It is the standard to represent real floating-point numbers.

### Upsides

It is the standard for modern hardware, and the majority of hardware FPU's use it today.

It is the most detailed and capable format of representation of "real" numbers, and has support for a variety of mechanisms such as infinity representation, not a number representation, and even invalid operation exception flagging.

It is used in industrial protocols such as Modbus.

### Downsides

Significantly harder to implement than other formats such as ASCII.

A value interpretation depends on its endianness, so one device may disagree with another if they have different endianness.

Since every value represents a number, no special values can be reliably defined to address functions or things of the sort.

Due to its complexity, it is much harder/slower for hardware to process.

## Fixed point representation

### What it is 

The sender device sends the fractional data as a whole number, while both the sender and receiver agree on the decimal placement.

### Upsides

It is generally faster to process than IEEE754 floating-point numbers, while not being as wasteful/inefficient as ASCII representation.
It sits on the middleground between the IEEE754 and ASCII representations in regards with the ease of implementation as well.
It tends to use less chip space and less power in digital signal processing.

### Downsides

It lacks precision and has a much more narrower representation interval depending on how many bytes specifies a number.

The finite precision introduces quantization errors that can lead to significant inaccuracies.

Arithmetic operations such as addition or multiplication have to carefully keep track of bit growth to prevent overflow and loss of precision.
