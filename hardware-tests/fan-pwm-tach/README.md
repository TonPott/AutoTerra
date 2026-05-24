# Fan PWM and tach

## Purpose

- Verify shared fan PWM driver on D6.
- Verify inverting 2N3904 PWM stage.
- Verify fan tach inputs on D9/D10.
- Verify approximate 25 kHz PWM generation strategy on Nano 33 IoT / SAMD21.
- Collect data for the PWM-to-RPM calibration table.

## Hardware under test

- Two Noctua NF-A4x20 5V PWM fans
- 2N3904 PWM driver
- External 2.2 kOhm PWM pull-up to +5 V
- External 10 kOhm tach pull-ups to 3.3 V
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- D6 -> 2.2 kOhm -> 2N3904 base
- 2N3904 emitter -> GND
- 2N3904 collector -> shared fan PWM node
- Shared fan PWM node -> 2.2 kOhm -> +5 V
- Both blue fan PWM wires -> shared fan PWM node
- Fan 1 green tach -> D9 with 10 kOhm pull-up to 3.3 V
- Fan 2 green tach -> D10 with 10 kOhm pull-up to 3.3 V
- Common GND required

## Planned sketch behavior

- Generate test PWM percentages.
- Account for the inverted hardware driver.
- Measure or at least print expected PWM settings.
- Count tach pulses.
- Compute RPM assuming two pulses per revolution.
- Print fan 1 RPM and fan 2 RPM.

## Expected result

- Fans respond to effective PWM percentage.
- Tach readings are plausible.
- Both fans can be measured independently.
- Data can be used later for calibration.

## Safety notes

- Do not connect the 5 V fan PWM node directly to a Nano GPIO.
- Tach pull-ups must be 3.3 V, not 5 V.

## Result notes

TODO: Add measured results after hardware test.
