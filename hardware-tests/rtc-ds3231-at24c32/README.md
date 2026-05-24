# RTC DS3231 and AT24C32

## Purpose

- Verify DS3231 timekeeping.
- Verify DS3231 INT/SQW on D2.
- Verify prepared DS3231 32kHz input on D8 without attaching a high-frequency interrupt.
- Verify AT24C32 EEPROM read/write through JC_EEPROM.

## Hardware under test

- DS3231 module with AT24C32
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- A4/A5 = I2C
- D2 = DS3231 INT/SQW, `INPUT_PULLUP`
- D8 = DS3231 32kHz, `INPUT_PULLUP`, optional/prepared

## Planned sketch behavior

- Read current RTC time.
- Optionally set RTC time if clearly requested in the sketch.
- Test alarm flag behavior on D2.
- Read/write a small test block in AT24C32.
- Verify that D8 can be read as a prepared input without using it for v1 logic.

## Expected result

- RTC time can be read.
- Alarm/INT behavior can be observed.
- EEPROM test data can be written and read back.
- 32kHz line is documented but not used as an interrupt in v1.

## Safety notes

- Confirm the DS3231 module exposes only 3.3 V safe signals to the Nano 33 IoT.
- Do not attach interrupts to the DS3231 32kHz input for v1 validation.

## Result notes

TODO: Add measured results after hardware test.
