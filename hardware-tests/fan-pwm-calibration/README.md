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

- Both fans started from stopped at 10% effective PWM.
- Fan 1 started at 8% effective PWM, but fan 2 did not.
- Neither fan started at 0% or 5% effective PWM.

Minimum stable running percent:

- Both fans kept running at 8% effective PWM after the 30% spin-up phase.
- Fan 1 kept running at 5% effective PWM, but fan 2 stopped.
- Neither fan kept running at 0% effective PWM.

Acoustic noise notes:

TODO: Add subjective noise notes after a dedicated acoustic check. No acoustic
notes were captured in this Serial-only run.

Stability notes:

- Fan 2 needed a higher startup percent than fan 1 in this run.
- Fan 2 also stopped at 5% during the stable-running test while fan 1 continued.
- The 5% calibration step from stopped produced 0 RPM for both fans.
- The 10% calibration step started both fans and settled around 405 RPM for fan 1 and 445 RPM for fan 2.
- The final production minimum should be confirmed again after the final PWM strategy is selected.

Summary:

| Finding | Effective PWM | Arduino PWM | Notes |
|---|---:|---:|---|
| Minimum startup, both fans | 10% | 230 | Fan 1 average 441.0 RPM, fan 2 average 685.5 RPM. |
| Minimum stable running, both fans | 8% | 235 | Fan 1 average 447.9 RPM, fan 2 average 439.3 RPM. |
| Fan 1 only stable running | 5% | 243 | Fan 1 average 447.9 RPM, fan 2 stopped. |

Calibration table paste area:

```text
STARTUP_RESULT,0,255,0.0,0.0,0,0
STARTUP_RESULT,5,243,0.0,0.0,0,0
STARTUP_RESULT,8,235,450.0,0.0,1,0
STARTUP_RESULT,10,230,441.0,685.5,1,1
STARTUP_RESULT,12,225,564.0,789.0,1,1
STARTUP_RESULT,15,217,763.5,987.0,1,1
STARTUP_RESULT,18,210,915.0,1138.5,1,1
STARTUP_RESULT,20,204,1062.0,1269.0,1,1

RUNNING_RESULT,20,204,1167.9,1230.0,1,1
RUNNING_RESULT,18,210,979.3,1022.1,1,1
RUNNING_RESULT,15,217,818.6,842.1,1,1
RUNNING_RESULT,12,225,610.7,615.0,1,1
RUNNING_RESULT,10,230,460.7,467.1,1,1
RUNNING_RESULT,8,235,447.9,439.3,1,1
RUNNING_RESULT,5,243,447.9,0.0,1,0
RUNNING_RESULT,0,255,2.1,0.0,0,0

CAL_RESULT,0,255,0.0,0.0,0.0,0.0,0.0,0.0
CAL_RESULT,5,243,0.0,0.0,0.0,0.0,0.0,0.0
CAL_RESULT,10,230,405.0,445.0,405.0,405.0,435.0,450.0
CAL_RESULT,15,217,760.0,780.0,750.0,765.0,780.0,780.0
CAL_RESULT,20,204,1090.0,1125.0,1080.0,1095.0,1125.0,1125.0
CAL_RESULT,25,192,1410.0,1460.0,1410.0,1410.0,1455.0,1470.0
CAL_RESULT,30,179,1700.0,1780.0,1695.0,1710.0,1770.0,1800.0
CAL_RESULT,35,166,1985.0,2070.0,1980.0,1995.0,2070.0,2070.0
CAL_RESULT,40,153,2270.0,2355.0,2265.0,2280.0,2355.0,2355.0
CAL_RESULT,45,141,2530.0,2605.0,2520.0,2535.0,2595.0,2610.0
CAL_RESULT,50,128,2760.0,2850.0,2760.0,2760.0,2850.0,2850.0
CAL_RESULT,55,115,3025.0,3120.0,3015.0,3030.0,3120.0,3120.0
CAL_RESULT,60,102,3265.0,3370.0,3255.0,3270.0,3360.0,3390.0
CAL_RESULT,65,90,3490.0,3585.0,3480.0,3495.0,3570.0,3600.0
CAL_RESULT,70,77,3735.0,3810.0,3720.0,3750.0,3810.0,3810.0
CAL_RESULT,75,64,3940.0,4045.0,3930.0,3945.0,4035.0,4065.0
CAL_RESULT,80,51,4150.0,4250.0,4140.0,4155.0,4245.0,4260.0
CAL_RESULT,85,39,4345.0,4470.0,4335.0,4350.0,4470.0,4470.0
CAL_RESULT,90,26,4460.0,4675.0,4425.0,4500.0,4665.0,4680.0
CAL_RESULT,95,13,4650.0,4840.0,4650.0,4650.0,4830.0,4845.0
CAL_RESULT,100,0,4880.0,5090.0,4875.0,4890.0,5085.0,5100.0
```
