# Telemetry Controller

## Goal
Build a small embedded telemetry system around an STM32.

The final system should:
1. sample environmental sensors periodically
2. timestamp measurements
3. transmit telemetry to a PC over serial
4. accept configuration commands from the PC
5. detect/report communication and sensor failures
6. recover from recoverable faults
7. eventually support RTOS-based scheduling and CAN

## Architecture

Sensor -> STM32 -> UART -> PC telemetry CLI

## Current status

- [ ] Host-side packet simulator
- [ ] Packet decoder
- [ ] MCU GPIO bring-up
- [ ] UART
- [ ] Sensor driver
- [ ] Telemetry
- [ ] Fault handling
- [ ] RTOS
- [ ] CAN

## Repository layout
firmware/
host/
tests/
docs/

## Design documentation
Detailed protocol and architectural decisions live under docs/.
