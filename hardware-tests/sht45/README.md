# SHT45

## Purpose

- Verify SHT45 communication with Sensirion arduino-i2c-sht4x.
- Read air temperature and humidity.
- Confirm selected I2C address.
- Observe error handling.

## Hardware under test

- SHT45 sensor
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- A4/A5 = I2C
- 3.3 V safe supply and pull-ups

## Planned sketch behavior

- Initialize SHT45.
- Read temperature and humidity.
- Print values periodically.
- Print errors clearly.

## Expected result

- Stable temperature and humidity readings.
- Selected address documented.
- Error behavior understood.

## Safety notes

- Confirm 3.3 V safe supply and I2C pull-ups before connecting the sensor.

## Result notes

TODO: Add measured results after hardware test.
