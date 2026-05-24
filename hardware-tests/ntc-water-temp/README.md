# NTC water temperature

## Purpose

- Verify B3950 10K NTC water temperature measurement.
- Verify NTC_Thermistor library behavior.
- Check the 8.2 kOhm reference resistor divider.

## Hardware under test

- B3950 10K NTC
- 8.2 kOhm reference resistor
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- 3.3 V -> 8.2 kOhm reference resistor -> A0 -> B3950 NTC -> GND
- AREF is not connected in v1

## Planned sketch behavior

- Read A0.
- Convert with NTC_Thermistor.
- Print Celsius, Fahrenheit, and Kelvin if supported.
- Print raw ADC value.
- Compare against a known thermometer if available.

## Expected result

- Plausible water temperature.
- Stable ADC readings.
- No need for external AREF unless later evidence shows unacceptable noise or drift.

## Safety notes

- Keep the analog divider referenced to 3.3 V and GND.
- Protect the board from water during probe handling.

## Result notes

TODO: Add measured results after hardware test.
