# Fan PWM frequency A/B

## Purpose

- Compare Arduino `analogWrite()` fan PWM against approximately 25 kHz PWM on Arduino Nano 33 IoT / SAMD21.
- Record RPM behavior for the same practical effective PWM steps in both modes.
- Provide a place to add manual acoustic notes for each mode.
- Help decide whether `analogWrite()` is acceptable for v1 or whether production firmware should implement approximately 25 kHz fan PWM.

This test does not replace the bring-up test in [`../fan-pwm-tach/`](../fan-pwm-tach/)
or the minimum/calibration test in [`../fan-pwm-calibration/`](../fan-pwm-calibration/).

## Hardware under test

- Two Noctua NF-A4x20 5V PWM fans
- D6 shared fan PWM driver output through an inverting 2N3904 stage
- D9 fan 1 tach input
- D10 fan 2 tach input
- External 10 kOhm tach pull-ups to 3.3 V
- External fan PWM pull-up to 5 V on the isolated transistor collector node
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Sketch

- `fan-pwm-frequency-ab.ino`

Run the compile check for this hardware test before uploading:

- Windows PowerShell:
  - `$env:SKETCH = "hardware-tests/fan-pwm-frequency-ab"`
  - `.\scripts\check-arduino.ps1`
- Linux/macOS:
  - `SKETCH=hardware-tests/fan-pwm-frequency-ab ./scripts/check-arduino.sh`

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

In `analogWrite()` mode, the summary line prints the Arduino PWM value. In
approximately 25 kHz mode, it prints the TCC0 compare value used for the same
inverted command.

## A/B comparison modes

The sketch uses compile-time mode selection. This avoids fragile runtime switching
back from local TCC0 configuration to Arduino core `analogWrite()` state.

Default mode:

```cpp
#define FAN_PWM_MODE FAN_PWM_MODE_ANALOGWRITE
```

To test the approximately 25 kHz mode, change the define in the sketch to:

```cpp
#define FAN_PWM_MODE FAN_PWM_MODE_25KHZ
```

Upload once per mode, copy the `AB_RESULT` lines from Serial Monitor, and add
manual acoustic notes beside the results.

## PWM implementation

- `ANALOGWRITE` mode uses Arduino `analogWrite()` on D6.
- `25KHZ` mode uses local SAMD21 TCC0 register code for D6.
- No external timer/PWM library is used.
- The Nano 33 IoT Arduino SAMD core maps D6 to `PA04`, `TCC0/WO0`, Peripheral E.
- The 25 kHz mode uses GCLK0 at 48 MHz, TCC0 prescaler DIV1, and `PER=1919`, which targets 25,000 Hz.
- The low-level timer code is local to this hardware test sketch and is not production firmware.

If D6 does not map to the expected TCC0 channel, the sketch prints a fatal error
and does not fake 25 kHz behavior.

## Test sequence

Each upload runs the same effective PWM sequence:

```text
0%, 8%, 10%, 15%, 20%, 30%, 50%, 75%, 100%
```

- Each step is held for 12 seconds.
- The first 6 seconds are treated as settling time.
- RPM status prints every 2 seconds.
- Summary averages use samples from the last 6 seconds of each step.

The selected steps check the known low running range, the measured minimum
startup value, quiet low-to-mid operation, and higher-speed behavior.

## CSV-style output

The sketch prints one result row per step:

```text
AB_RESULT,mode,percent,pwm_or_compare_value,fan1_avg_rpm,fan2_avg_rpm,fan1_min_rpm,fan1_max_rpm,fan2_min_rpm,fan2_max_rpm,manual_noise_note
```

The `manual_noise_note` field is intentionally empty. Add notes manually after
listening, for example:

- `quiet`
- `slight ticking`
- `audible hum`
- `worse than analogWrite`
- `better than analogWrite`

Firmware does not measure sound.

## Decision criteria

Use this test to decide whether `analogWrite()` is acceptable for v1 or whether
approximately 25 kHz PWM should be implemented in production firmware.

Compare:

- acoustic behavior,
- stable operation at 8%, 10%, 15%, 20%, and 30%,
- RPM differences at the same effective percentages,
- implementation complexity and timer side effects.

The final production PWM-to-RPM calibration table must be repeated or confirmed
after the final PWM strategy is selected.

## Result notes

analogWrite acoustic notes:

TODO: Add manual listening notes. No acoustic notes were captured in this
Serial-only result set.

25 kHz acoustic notes:

TODO: Add manual listening notes. No acoustic notes were captured in this
Serial-only result set.

RPM comparison:

The A/B test ran successfully in both compile-time modes. The approximately
25 kHz mode used local TCC0 code and produced tach/RPM data on both fans.

At the same effective percent, the approximately 25 kHz mode generally produced
lower RPM than `analogWrite()` in the low-to-mid range and similar RPM in the
higher range. At 100%, the approximately 25 kHz mode was slightly higher in this
run.

Low-percent behavior differed:

- In `analogWrite()` mode, fan 1 started at 8% and fan 2 started at 10%.
- In approximately 25 kHz mode, neither fan started at 8%, fan 1 started at 10%, and both fans started at 15%.

Summary table:

| Effective PWM | analogWrite fan 1 RPM | analogWrite fan 2 RPM | 25 kHz fan 1 RPM | 25 kHz fan 2 RPM | Notes |
|---:|---:|---:|---:|---:|---|
| 0% | 0.0 | 0.0 | 0.0 | 0.0 | Both modes off. |
| 8% | 416.3 | 0.0 | 0.0 | 0.0 | Only analogWrite fan 1 moved. |
| 10% | 393.8 | 536.2 | 442.5 | 0.0 | 25 kHz fan 2 did not start. |
| 15% | 768.7 | 810.0 | 585.0 | 735.0 | Both modes ran both fans. |
| 20% | 1117.5 | 1147.5 | 930.0 | 1038.7 | 25 kHz lower. |
| 30% | 1751.2 | 1800.0 | 1638.7 | 1698.7 | 25 kHz lower. |
| 50% | 2820.0 | 2872.5 | 2752.5 | 2782.5 | Similar, 25 kHz slightly lower. |
| 75% | 3911.3 | 3956.3 | 3903.8 | 3922.5 | Very similar. |
| 100% | 4886.3 | 4957.5 | 4965.0 | 5002.5 | 25 kHz slightly higher. |

Raw result lines:

```text
AB_RESULT,ANALOGWRITE,0,255,0.0,0.0,0.0,0.0,0.0,0.0,
AB_RESULT,ANALOGWRITE,8,235,416.3,0.0,405.0,435.0,0.0,0.0,
AB_RESULT,ANALOGWRITE,10,230,393.8,536.2,390.0,405.0,465.0,690.0,
AB_RESULT,ANALOGWRITE,15,217,768.7,810.0,750.0,780.0,810.0,810.0,
AB_RESULT,ANALOGWRITE,20,204,1117.5,1147.5,1095.0,1140.0,1140.0,1155.0,
AB_RESULT,ANALOGWRITE,30,179,1751.2,1800.0,1740.0,1755.0,1770.0,1815.0,
AB_RESULT,ANALOGWRITE,50,128,2820.0,2872.5,2820.0,2820.0,2850.0,2880.0,
AB_RESULT,ANALOGWRITE,75,64,3911.3,3956.3,3885.0,3930.0,3945.0,3975.0,
AB_RESULT,ANALOGWRITE,100,0,4886.3,4957.5,4860.0,4920.0,4950.0,4965.0,

AB_RESULT,25KHZ,0,1919,0.0,0.0,0.0,0.0,0.0,0.0,
AB_RESULT,25KHZ,8,1768,0.0,0.0,0.0,0.0,0.0,0.0,
AB_RESULT,25KHZ,10,1731,442.5,0.0,420.0,465.0,0.0,0.0,
AB_RESULT,25KHZ,15,1633,585.0,735.0,570.0,600.0,660.0,885.0,
AB_RESULT,25KHZ,20,1535,930.0,1038.7,915.0,945.0,1020.0,1050.0,
AB_RESULT,25KHZ,30,1347,1638.7,1698.7,1620.0,1650.0,1680.0,1710.0,
AB_RESULT,25KHZ,50,963,2752.5,2782.5,2745.0,2760.0,2775.0,2790.0,
AB_RESULT,25KHZ,75,482,3903.8,3922.5,3885.0,3915.0,3900.0,3945.0,
AB_RESULT,25KHZ,100,0,4965.0,5002.5,4950.0,4980.0,4980.0,5010.0,
```

Final decision:

TODO: Undecided. The RPM comparison alone does not justify switching to 25 kHz
for v1. Record manual acoustic notes and, if possible, measure the actual PWM
waveform before deciding whether `analogWrite()` is acceptable or 25 kHz PWM is
required.
