# TCS34725

## Purpose

- Verify TCS34725 / AngelTCS34725US communication.
- Read Clear raw value.
- Test LED control on D4.
- Prepare optional INT behavior on D7.

## Hardware under test

- AngelTCS34725US / TCS34725FN
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- A4/A5 = I2C
- D4 = LED control, LOW off, HIGH on
- D7 = INT, open-drain, `INPUT_PULLUP`

## Planned sketch behavior

- Initialize sensor.
- Print raw Clear value.
- Optionally print RGB, lux, and color temperature.
- Toggle LED using D4.
- Observe the difference between LED off/on if useful.
- Read D7 state without depending on INT for v1.

## Expected result

- Clear raw values are readable.
- LED control works as documented.
- Useful on/off threshold behavior can be evaluated in the real room.

## Safety notes

- Confirm I2C and INT lines are 3.3 V safe.
- Avoid looking directly into bright LEDs during close-up testing.

## Result notes

TODO: Add measured results after hardware test.
