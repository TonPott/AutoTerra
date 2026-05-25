# Fan PWM and tach

## Purpose

- Verify shared fan PWM driver on D6.
- Verify inverting 2N3904 PWM stage.
- Verify fan tach inputs on D9/D10.
- Verify RPM calculation from tach pulses.
- Print effective fan percent, Arduino PWM value, pulse counts, and measured RPM.

This first manual test uses Arduino `analogWrite()` for a basic wiring and tach
bring-up. It does not validate the final approximately 25 kHz fan PWM
requirement. The final SAMD21 timer setup and PWM-to-RPM calibration table remain
separate validation tasks.

For minimum startup, minimum stable running, and finer 5% step RPM data, use the
separate calibration sketch in [`../fan-pwm-calibration/`](../fan-pwm-calibration/).

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

## Sketch

- `fan-pwm-tach.ino`

Run the compile check for this hardware test before uploading:

- Windows PowerShell:
  - `$env:SKETCH = "hardware-tests/fan-pwm-tach"`
  - `.\scripts\check-arduino.ps1`
- Linux/macOS:
  - `SKETCH=hardware-tests/fan-pwm-tach ./scripts/check-arduino.sh`

Upload the sketch to the Arduino Nano 33 IoT with the normal Arduino tooling,
then open Serial Monitor at `115200` baud.

## Wiring summary

- D6 -> 2.2 kOhm -> 2N3904 base
- 2N3904 emitter -> GND
- 2N3904 collector -> shared fan PWM node
- Shared fan PWM node -> 2.2 kOhm -> +5 V
- Both blue fan PWM wires -> shared fan PWM node
- Fan 1 green tach -> D9 with 10 kOhm pull-up to 3.3 V
- Fan 2 green tach -> D10 with 10 kOhm pull-up to 3.3 V
- Common GND required

## Pins

- D6: shared fan PWM driver output
- D9: fan 1 tach input
- D10: fan 2 tach input

## PWM inversion

The user-facing fan command is an effective fan percent. The sketch converts that
effective percent to an inverted Arduino PWM value before writing D6 because the
2N3904 collector stage inverts the signal:

- effective 0% -> Arduino PWM 255
- effective 100% -> Arduino PWM 0

The Serial output prints both the effective fan percent and the Arduino PWM value
written to D6.

## Test sequence

- The sequence repeats continuously: 0%, 15%, 25%, 50%, 75%, 100%.
- Each step is held for 10 seconds.
- Tach reports print every 2 seconds.
- RPM assumes two tach pulses per revolution unless hardware testing proves otherwise.
- Tach interrupts count falling edges on D9 and D10.
- D9 and D10 use external pull-ups only; the sketch does not enable internal pull-ups.

## Expected result

- Fans respond to effective PWM percentage.
- Tach readings are plausible.
- Both fans can be measured independently.
- Data can be used later for calibration.

## Safety notes

- Do not connect the 5 V fan PWM node directly to a Nano GPIO.
- The fan PWM node is pulled up to 5 V but isolated from D6 by the 2N3904 collector node.
- Tach pull-ups must be 3.3 V, not 5 V.
- Fan tach outputs must remain separate and must not be tied together.
- Common GND is required between the Nano, fan supply, fan grounds, and transistor emitter.

This test intentionally does not change production firmware, library versions, or
build scripts.

## Result notes

Initial manual run with `analogWrite()` confirmed that D6 inversion, the shared
PWM driver, and both separate tach inputs are functional.

- Tach interrupts attached successfully on D9 and D10.
- Effective 0% with Arduino PWM 255 stopped both fans after spin-down.
- Tach pulses were visible during spin-down after switching from 100% to 0%.
- Both fans responded to increasing effective PWM percentage.
- Fan 1 and fan 2 RPM readings were measured independently.

Approximate settled readings from two repeated stepped runs:

| Effective fan percent | Arduino PWM written to D6 | Fan 1 settled RPM | Fan 2 settled RPM | Notes |
|---:|---:|---:|---:|---|
| 0% | 255 | 0 | 0 | Fans stop after spin-down. |
| 15% | 217 | 825-840 | 855-900 | Fan 2 showed a higher transient RPM while stabilizing. |
| 25% | 192 | 1500-1530 | 1500-1530 | Both fans settled closely together. |
| 50% | 128 | 2865-2895 | 2895-2925 | Both fans settled closely together. |
| 75% | 64 | 3945-3960 | 3990-4110 | Fan 2 was slightly faster in this run. |
| 100% | 0 | 4860-5040 | 4980-5010 | Full-speed readings are plausible for the selected fans. |

These readings are not the final PWM-to-RPM calibration table because this test
uses Arduino `analogWrite()` and does not validate the final approximately
25 kHz PWM requirement.
