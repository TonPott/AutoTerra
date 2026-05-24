# Liquid sensor

## Purpose

- Verify CQRobot multi-point liquid level sensor frequency output.
- Confirm `INPUT_PULLUP` behavior on D5.
- Measure nominal frequencies.
- Evaluate real-world fluctuation.

## Hardware under test

- CQRobot multi-point photoelectric liquid level sensor
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- Sensor signal -> D5
- D5 = `INPUT_PULLUP`
- Sensor output must be 3.3 V compatible

## Planned sketch behavior

- Measure frequency.
- Classify nominal states:
  - Without liquid: 20 Hz
  - Level 1: 50 Hz
  - Level 2: 100 Hz
  - Level 3: 200 Hz
  - Level 4: 400 Hz
- Print raw frequency and classified level.
- Allow collecting data for tolerance and stabilization logic.

## Expected result

- Measured frequencies roughly match documented values.
- Real fluctuation can be documented.
- Behavior near water boundaries can be observed.

## Safety notes

- Confirm the signal output is 3.3 V compatible before connecting it to D5.
- Keep sensor wiring secured before testing near water.

## Result notes

TODO: Add measured results after hardware test.
