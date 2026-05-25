# Fan PWM calibration

## Purpose

- Find the minimum effective PWM percent where both fans start from stopped.
- Find the minimum effective PWM percent where already spinning fans keep running.
- Collect finer `analogWrite()`-based RPM data in 5% effective PWM steps.
- Leave space for acoustic and stability notes from the real hardware test.
- Keep final approximately 25 kHz PWM validation explicitly open.

This is a second fan hardware test. The first bring-up sketch remains in
[`../fan-pwm-tach/`](../fan-pwm-tach/) and is used to validate basic wiring,
driver inversion, and tach reading.

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

- `fan-pwm-calibration.ino`

Run the compile check for this hardware test before uploading:

- Windows PowerShell:
  - `$env:SKETCH = "hardware-tests/fan-pwm-calibration"`
  - `.\scripts\check-arduino.ps1`
- Linux/macOS:
  - `SKETCH=hardware-tests/fan-pwm-calibration ./scripts/check-arduino.sh`

Upload the sketch to the Arduino Nano 33 IoT with the normal Arduino tooling,
then open Serial Monitor at `115200` baud.

## Wiring summary

- D6 -> 2.2 kOhm -> 2N3904 base
- 2N3904 emitter -> GND
- 2N3904 collector -> shared fan PWM node
- Shared fan PWM node -> 2.2 kOhm -> +5 V
- Both blue fan PWM wires -> shared fan PWM node
- Fan yellow wires -> +5 V
- Fan black wires -> common GND
- Fan 1 green tach -> D9 with 10 kOhm pull-up to 3.3 V
- Fan 2 green tach -> D10 with 10 kOhm pull-up to 3.3 V
- Common GND required

## Safety notes

- Do not connect the 5 V fan PWM node directly to a Nano GPIO.
- The fan PWM node is pulled up to 5 V but isolated from D6 by the 2N3904 collector node.
- Tach pull-ups must be 3.3 V, not 5 V.
- Fan tach outputs must remain separate and must not be tied together.
- D9 and D10 use external pull-ups only; the sketch does not enable internal pull-ups.
- Common GND is required between the Nano, fan supply, fan grounds, and transistor emitter.

## PWM inversion

The sketch works in effective fan percent and converts that value before writing
to D6 because the 2N3904 collector stage is inverting:

- effective 0% -> Arduino PWM 255
- effective 100% -> Arduino PWM 0

The Serial output prints both the effective fan percent and the Arduino PWM value
written to D6.

## Test phases

### Minimum startup test

Purpose: find the lowest effective PWM percent where both fans can start from
stopped.

- Candidates: 0%, 5%, 8%, 10%, 12%, 15%, 18%, 20%.
- Before each candidate, the sketch commands effective 0%.
- Spin-down time before each candidate: 15 seconds.
- Candidate observation time: 20 seconds.
- RPM status prints every 2 seconds.
- The summary threshold for `started` is average RPM > 100.

### Minimum stable running test

Purpose: find the lowest effective PWM percent where already spinning fans keep
running.

- The sketch spins both fans at 30% for 10 seconds before the decreasing candidate sequence.
- Candidates: 20%, 18%, 15%, 12%, 10%, 8%, 5%, 0%.
- Candidate hold time: 15 seconds.
- RPM status prints every 2 seconds.
- The summary threshold for `running` is average RPM > 100.

### 5%-step calibration sweep

Purpose: collect `analogWrite()`-based RPM data in 5% effective PWM steps.

- Steps: 0%, 5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, 50%, 55%, 60%, 65%, 70%, 75%, 80%, 85%, 90%, 95%, 100%.
- Hold time per step: 15 seconds.
- The first 6 seconds are treated as settling time.
- Summary averages use samples from the last 6 seconds of each step.
- RPM status prints every 2 seconds.

This sweep is not the final production PWM-to-RPM calibration table unless the
final PWM strategy is still equivalent after the approximately 25 kHz PWM work is
completed.

## CSV-style output

The sketch prints periodic status rows and one result row per candidate or step.
Result rows are intended to be easy to copy into a table or spreadsheet.

- `STARTUP_RESULT,percent,pwm,fan1_avg_rpm,fan2_avg_rpm,fan1_started,fan2_started`
- `RUNNING_RESULT,percent,pwm,fan1_avg_rpm,fan2_avg_rpm,fan1_running,fan2_running`
- `CAL_RESULT,percent,pwm,fan1_avg_rpm,fan2_avg_rpm,fan1_min_rpm,fan1_max_rpm,fan2_min_rpm,fan2_max_rpm`

Boolean fields use `1` for true and `0` for false.

## Assumptions and limits

- RPM calculation assumes two tach pulses per revolution unless hardware testing proves otherwise.
- This sketch uses Arduino `analogWrite()` only.
- This sketch does not validate the final approximately 25 kHz PWM requirement.
- The final production PWM-to-RPM calibration table must be repeated or confirmed after the final PWM strategy is selected.

## Result notes

Minimum startup percent:

TODO: Add measured result after hardware test.

Minimum stable running percent:

TODO: Add measured result after hardware test.

Acoustic noise notes:

TODO: Add subjective noise notes after hardware test.

Stability notes:

TODO: Add observed startup, stalling, oscillation, or mismatch notes after hardware test.

Calibration table paste area:

```text
TODO: Paste STARTUP_RESULT, RUNNING_RESULT, and CAL_RESULT lines from Serial output.
```
