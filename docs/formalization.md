# Telemetry Protocol

STM32 ----- UART ----- PC

The STM32 periodically produces something conceptually like:

> temperature = 23.7 C
> pressure = 100812 Pa
> timestamp = ...

The PC must reliably receive those measurements.

The job is to answer these questions:

How to represent the numbers?
Could be:

1. ASCII
2. IEEE-754 floating point
3. Signed integer representing thousandths

What are the advantages and disadvantages?
Don't assume one is universally right.

---

# Message boundaries

UART gives you **bytes**.

You receive:

> 01 FA A2 44 7B 19 ...

Where does one message begin?
Where does it end?
What happens if the PC connects while the STM32 is already halfway through transmitting something?

Research:

1. Delimiter-based framing
2. Length-prefixed framing
3. Fixed-size messages
4. Sentinel/magic bytes
5. COBS/SLIP

You don't need all of them.
Understand why they exist.

---

# Corruption

Suppose one bit changes:

> temperature = 23.7

becomes:

> temperature = 87.4

How would you know?

Research:

1. Parity
2. Checksum
3. CRC

---

# Lost messages

Suppose the MCU sends:

> measurements
> measurements
> measurements
> measurements
> measurements

and the PC sees three.

How can it know something disappeared?

That's where the idea of a sequence number should emerge naturally.
Maybe you decide you don't care.
That's also a legitimate design choice if you can explain it.

---

# Time

The PC receives a measurement at 10:43:12.

Was that when the sensor was sampled?
Or merely when the PC received it?

Should the timestamp come from:
1. STM32 uptime?
2. milliseconds since boot?
3. Unix time?
4. The PC?

What happens if the MCU doesn't have a real-time clock?
Again, there's no universal answer.

---

# Different message types

Eventually the MCU may send:

> measurement
> error
> status
> debug

and the PC might send:

> set sampling rate
> reset counters
> request status

How does the receiver distinguish those?
That's what eventually motivates something like a message type field.
