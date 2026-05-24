# I2C bus scan

## Purpose

- Verify the I2C bus on A4/A5.
- Detect attached I2C devices.
- Confirm that SDA/SCL pull-ups are effective and 3.3 V safe.

## Hardware under test

- Arduino Nano 33 IoT
- DS3231 RTC
- AT24C32 EEPROM
- SHT45
- TCS34725

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Sketch

- [`i2c-bus-scan.ino`](i2c-bus-scan.ino)

The sketch uses only the Arduino `Wire.h` library. It initializes the Nano 33 IoT I2C bus on A4/A5, scans addresses `0x03` through `0x77`, prints detected devices, and repeats the scan every 5 seconds.

## Wiring summary

- A4 = SDA
- A5 = SCL
- Effective pull-ups to 3.3 V
- No 5 V pull-ups on SDA/SCL

## How to run

1. Confirm that SDA and SCL are pulled up only to 3.3 V.
2. Connect the Nano 33 IoT and the I2C modules with common ground.
3. Open `hardware-tests/i2c-bus-scan/i2c-bus-scan.ino` in Arduino IDE, or compile/upload it as a standalone sketch with Arduino CLI.
4. Select board `Arduino Nano 33 IoT`.
5. Upload the sketch.
6. Open Serial Monitor at `115200` baud.

## Sketch behavior

- Scan I2C addresses.
- Print detected addresses.
- Print likely or typical AutoTerra device hints for known addresses.
- Print a warning if no devices are found.
- Repeat the scan every 5 seconds using `millis()`.

## Typical address hints

Exact addresses must be confirmed on the real hardware.

| Address | Typical device |
|---|---|
| `0x29` | TCS34725 color sensor |
| `0x44` / `0x45` | SHT45 / SHT4x depending on module/address configuration |
| `0x57` | AT24C32 EEPROM |
| `0x60` | Onboard ATECC608A secure element on Nano 33 IoT |
| `0x68` | DS3231 RTC |
| `0x6A` | Onboard LSM6DS3 IMU on Nano 33 IoT |

## Expected result

- I2C devices are detected.
- Bus scan does not hang.
- Detected addresses are documented in the result notes.

## Safety notes

- Confirm that all I2C pull-ups are 3.3 V safe before connecting the bus to the Nano 33 IoT.
- Disconnect or correct any module that pulls SDA or SCL to 5 V.
- Effective pull-ups to 3.3 V are required for reliable I2C operation.

## Result notes

Manual test result:

| Address | Observed likely device |
|---|---|
| `0x29` | TCS34725 color sensor |
| `0x44` | SHT45 / SHT4x air temperature and humidity sensor |
| `0x57` | AT24C32 EEPROM |
| `0x60` | Onboard ATECC608A secure element on Nano 33 IoT |
| `0x68` | DS3231 RTC |
| `0x6A` | Onboard LSM6DS3 IMU on Nano 33 IoT |

The expected external devices were found at the expected addresses. The SHT45 was found at `0x44`.

The additional `0x60` and `0x6A` devices are likely onboard devices on the Arduino Nano 33 IoT, not external AutoTerra modules.
