# RTC DS3231 and AT24C32

## Purpose

Manual hardware test for the DS3231 RTC module with integrated AT24C32 EEPROM.

The sketch file is:

- [`rtc-ds3231-at24c32.ino`](rtc-ds3231-at24c32.ino)

This test verifies:

- DS3231 communication and current time readout.
- DS3231 lost-power status when exposed by RTClib.
- DS3231 Alarm1 / INT/SQW behavior on D2.
- Prepared DS3231 32kHz input wiring on D8 without attaching a high-frequency interrupt.
- AT24C32 EEPROM read/write through JC_EEPROM.

## Hardware under test

- DS3231 module with AT24C32
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- A4/A5 = I2C SDA/SCL
- D2 = DS3231 INT/SQW, `INPUT_PULLUP`
- D8 = DS3231 32kHz, `INPUT_PULLUP`, optional/prepared

Expected I2C addresses:

| Address | Device |
|---|---|
| `0x68` | DS3231 RTC |
| `0x57` | AT24C32 EEPROM |

The previous `i2c-bus-scan` test already confirmed `0x68` and `0x57` on the assembled AutoTerra I2C bus. It also confirmed `0x44` for SHT45, `0x29` for TCS34725, and likely onboard Nano 33 IoT devices at `0x60` / `0x6A`.

## How to run

Baud rate:

- `115200`

Compile check:

- Windows PowerShell:
  - `$env:SKETCH = "hardware-tests/rtc-ds3231-at24c32"`
  - `.\scripts\check-arduino.ps1`
- Linux/macOS:
  - `SKETCH=hardware-tests/rtc-ds3231-at24c32 ./scripts/check-arduino.sh`

Upload the sketch to the Arduino Nano 33 IoT, then open Serial Monitor at `115200` baud.

## Sketch behavior

- Read current RTC time.
- Print whether DS3231 communication succeeded.
- Print whether the RTC reports lost power, when RTClib exposes that status.
- Leave RTC time unchanged by default.
- Optionally set RTC time from build time if the compile-time flag is deliberately changed:

```cpp
constexpr bool SET_RTC_FROM_COMPILE_TIME = false;
```

- Print a strong warning when RTC time setting is enabled.
- Clear existing alarm flags if RTC communication is available.
- Configure DS3231 INT/SQW for alarm output.
- Set Alarm1 roughly 60 seconds in the future.
- Poll D2 and the DS3231 alarm flag without attaching an interrupt handler.
- Clear and disable Alarm1 after it fires, if possible.
- Read D8 as a prepared optional 32kHz input.
- Do not attach an interrupt to D8.
- Do not count 32kHz pulses.
- Verify the AT24C32 at expected address `0x57`.
- Write and read one small test record through JC_EEPROM.
- Print RTC time and D2/D8 states every 5 seconds using `millis()`.

## EEPROM write note

This sketch writes a small test structure to the AT24C32 EEPROM once during startup.

The EEPROM test offset is:

- `0x0F00`

The AT24C32 is 32 kbit / 4096 bytes. Offset `0x0F00` is near the end of the address space and is intended to avoid future production configuration areas. The sketch does not erase the EEPROM, does not scan the full EEPROM, and does not write repeatedly in the main loop.

## Expected result

- RTC time can be read.
- Alarm/INT behavior can be observed.
- D2 reads normally HIGH and should go LOW when Alarm1 fires.
- EEPROM test data can be written and read back from `0x0F00`.
- D8 can be read as a prepared input, but is not used as an interrupt in v1.

Serial output should show:

- RTC initialization status
- current RTC time
- RTC lost-power status, if available
- D2 INT/SQW pin state
- D8 32kHz prepared input state
- alarm setup status
- alarm fired status
- EEPROM write/read verification result

## Safety notes

- Confirm the DS3231 module exposes only 3.3 V safe signals to the Nano 33 IoT.
- Do not attach interrupts to the DS3231 32kHz input for v1 validation.
- This test writes to AT24C32 EEPROM offset `0x0F00`; do not run it on hardware where that address must be preserved.

## Result notes

Manual test result, 2026-05-25:

- RTC time read succeeded.
- Example RTC time near alarm trigger: `2026-05-25 10:42:23`.
- RTC lost-power flag at startup was `false`.
- DS3231 32kHz output enabled flag at startup was `true`.
- D8 32kHz prepared input read `HIGH`; no interrupt was attached and no 32kHz pulse count was attempted.
- Alarm1 setup through RTClib succeeded.
- Before the alarm trigger, D2 INT/SQW read `HIGH`.
- At the Alarm1 trigger, the DS3231 alarm flag was set and D2 INT/SQW read `LOW`.
- After clearing and disabling Alarm1, D2 INT/SQW returned to `HIGH`.
- AT24C32 address check passed at `0x57`.
- EEPROM write/read verification passed at test offset `0x0F00`.

Open follow-up:

- 32kHz pulse behavior on D8 was not measured by this test.
- I2C and DS3231 signal voltage levels still depend on direct electrical measurement.
