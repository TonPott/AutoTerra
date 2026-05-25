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

TODO: Add manual listening notes after the analogWrite upload.

25 kHz acoustic notes:

TODO: Add manual listening notes after the 25 kHz upload.

RPM comparison:

TODO: Paste and compare `AB_RESULT` lines from both uploads.

Final decision:

TODO: Undecided. Decide later whether analogWrite is acceptable for v1 or whether 25 kHz PWM is required.
