# I2C bus scan

## Purpose

- Verify the I2C bus on A4/A5.
- Detect attached I2C devices.
- Confirm that SDA/SCL pull-ups are effective and 3.3 V safe.

## Hardware under test

- Arduino Nano 33 IoT
- DS3231 RTC
- AT24C32 EEPROM
- SHT45
- TCS34725

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- A4 = SDA
- A5 = SCL
- Effective pull-ups to 3.3 V
- No 5 V pull-ups on SDA/SCL

## Planned sketch behavior

- Scan I2C addresses.
- Print detected addresses.
- Optionally identify expected devices by address range or known address.
- Print a warning if no devices are found.

## Expected result

- I2C devices are detected.
- Bus scan does not hang.
- Detected addresses are documented in the result notes.

## Safety notes

- Confirm that all I2C pull-ups are 3.3 V safe before connecting the bus to the Nano 33 IoT.
- Disconnect or correct any module that pulls SDA or SCL to 5 V.

## Result notes

TODO: Add measured results after hardware test.
