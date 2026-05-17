# HARDWARE.md – Physical build and wiring overview

## Purpose

This document is the central physical wiring reference for AutoTerra.

It summarizes power domains, pin assignments, module wiring, and signal-level rules for building or reviewing the hardware. It does not replace the functional requirements in `SPEC.md`, the module responsibilities in `MODULES.md`, or the validation tracking in `ROADMAP.md`.

## System overview

Arduino Nano 33 IoT is the only target board.

The controller monitors and controls a terrarium with light, fans, water level, water temperature, air climate, RTC, EEPROM, pump relay, and Home Assistant connectivity.

The Arduino performs local hardware control and safety logic. Home Assistant provides configuration, dashboard, schedule triggers, and notifications, but pump safety and hardware-facing control remain local.

## Power domains

- A shared 5 V supply powers the fans and the Arduino Nano 33 IoT through VIN.
- All grounds must be common: supply GND, Nano GND, fan grounds, sensor grounds, relay module ground, and transistor emitter ground.
- Nano 33 IoT logic is 3.3 V.
- All signals connected directly to Nano pins must be 3.3 V compatible.
- I2C pull-ups must not expose Nano pins to 5 V.
- The fan PWM line may be pulled up to 5 V only because it is isolated from the Nano GPIO by the 2N3904 transistor collector node.
- Fan tach pull-ups must go to 3.3 V.

## Complete pin map

```text
D2   DS3231 INT/SQW                 INPUT_PULLUP
D3   IR LED driver                  OUTPUT / handled by IRremote
D4   TCS34725 LED control           OUTPUT, default LOW = LED off, HIGH = LED on
D5   water level frequency input    INPUT_PULLUP
D6   shared fan PWM driver          OUTPUT, inverted through 2N3904
D7   TCS34725 INT                   INPUT_PULLUP, open-drain, optional / prepared
D8   DS3231 32kHz output            INPUT_PULLUP, optional / prepared
D9   fan 1 tach                     INPUT, external 10 kΩ pull-up to 3.3 V
D10  fan 2 tach                     INPUT, external 10 kΩ pull-up to 3.3 V
D11  pump relay control             OUTPUT, safe default pump off
A0   NTC water temperature          analog input
A4   I2C SDA                        DS3231 / AT24C32 / SHT45 / TCS34725
A5   I2C SCL                        DS3231 / AT24C32 / SHT45 / TCS34725
```

## I2C bus

The I2C bus is on A4/A5.

Devices on the bus:

- DS3231 RTC
- AT24C32 EEPROM on the RTC module
- SHT45 air temperature and humidity sensor
- TCS34725 light/color sensor

I2C addresses are finalized during hardware and module tests. The SHT45 address is intentionally hardcoded later.

The bus must remain 3.3 V safe for the Nano 33 IoT. I2C module supply and pull-ups must not expose Nano pins to 5 V unless proper level shifting or isolation is used.

## Module wiring overview

| Module | Power | Signal pins | Notes |
|---|---|---|---|
| DS3231 + AT24C32 | 3.3 V safe module supply | A4 SDA, A5 SCL, D2 INT/SQW, D8 32kHz optional | RTC alarms use D2. AT24C32 shares I2C. D8 is prepared only. |
| SHT45 | 3.3 V safe module supply | A4 SDA, A5 SCL | Air temperature and humidity. Address is hardcoded later. |
| TCS34725 AngelTCS34725US / TCS34725FN | 3.3 V safe module supply | A4 SDA, A5 SCL, D4 LED, D7 INT optional | D4 LOW = LED off, HIGH = LED on. INT is open-drain and uses INPUT_PULLUP. |
| B3950 10K NTC | 3.3 V divider supply | A0 | 3.3 V -> 8.2 kΩ reference resistor -> A0 -> B3950 NTC -> GND. |
| CQRobot multi-point liquid level sensor | Sensor supply per module requirements, output must be 3.3 V safe | D5 | Frequency-output sensor, not I2C. D5 uses INPUT_PULLUP. |
| Two Noctua NF-A4x20 5V PWM fans | +5 V fan supply | D6 PWM driver, D9 fan 1 tach, D10 fan 2 tach | Shared 2N3904 PWM driver, separate tach inputs. |
| Pump relay module | Relay module supply per module requirements | D11 | D11 commands relay input. Safe default is pump off. |
| IR LED driver | Driver supply per circuit requirements | D3 | Driven by IRremote. Detailed driver circuit may be documented later. |

## Fan wiring

Fan power:

- both fan yellow wires connect to +5 V,
- both fan black wires connect to common GND.

Shared PWM driver:

- both blue PWM wires connect to the shared fan PWM collector node,
- the shared PWM collector node is pulled up to +5 V with 2.2 kΩ,
- D6 drives the 2N3904 base through 2.2 kΩ,
- 2N3904 emitter connects to GND,
- 2N3904 collector connects to the shared fan PWM node,
- 10 kΩ base pulldown connects base to GND,
- the driver is inverting.

Tach wiring:

- fan 1 green tach wire connects to D9,
- fan 2 green tach wire connects to D10,
- each tach line has its own 10 kΩ pull-up to 3.3 V,
- tach outputs must not be tied together,
- tach pull-ups must not connect to 5 V.

## Water level sensor wiring

- Signal output connects to D5.
- D5 is configured as INPUT_PULLUP.
- The sensor is a frequency-output sensor, not I2C.
- The sensor output connected to D5 must be 3.3 V compatible.

## Pump relay wiring

- Relay control input connects to D11.
- D11 is configured as OUTPUT.
- The safe default is pump off.
- Pump cutoff is local and latched.
- Firmware only commands the relay module.
- Mains wiring, isolation, enclosure, strain relief, and safety are hardware responsibilities.
- Do not assume a GPIO can drive a relay coil directly; the relay module must provide suitable driver circuitry.

## Light sensor wiring

The AngelTCS34725US / TCS34725FN module uses I2C plus LED and INT pins:

- SDA connects to A4,
- SCL connects to A5,
- LED control connects to D4,
- D4 HIGH = LED on,
- D4 LOW = LED off,
- INT connects to D7,
- INT is open-drain,
- D7 uses INPUT_PULLUP,
- INT is optional/prepared and not required for v1 logic.

## RTC and EEPROM wiring

- DS3231 and AT24C32 share I2C on A4/A5.
- DS3231 INT/SQW connects to D2 with INPUT_PULLUP.
- DS3231 32kHz output connects to D8 with INPUT_PULLUP.
- D8 is optional/prepared.
- No interrupt shall be attached to DS3231 32kHz in v1.

## Analog water temperature wiring

- B3950 10K NTC connects to A0 through a voltage divider.
- The divider uses an 8.2 kΩ upper reference resistor from 3.3 V to A0.
- Nominal thermistor resistance is 10 kΩ.
- B value is 3950.
- Nominal temperature is 25 °C.

Wiring:

```text
3.3 V -> 8.2 kΩ -> A0 -> B3950 NTC -> GND
```

The 8.2 kΩ resistor is the upper reference leg, and the NTC is the lower leg to GND.

## IR output wiring

- IR LED driver is assigned to D3.
- D3 is handled by IRremote.
- The detailed IR LED driver circuit may be documented later if needed.
- Do not move D3 unless explicitly decided.

## Signal-level rules

- Nano 33 IoT GPIO is 3.3 V logic.
- Direct Nano inputs must not be pulled to 5 V.
- I2C pull-ups must be 3.3 V safe.
- Fan tach pull-ups must be 3.3 V.
- Fan PWM pull-up may be 5 V only at the isolated transistor collector / fan PWM node.
- All grounds must be common.

## Safety notes

- Pump mains voltage must be treated as dangerous.
- Firmware documentation cannot replace safe electrical construction.
- Relay/contact rating is not the only safety requirement.
- Enclosure, isolation, strain relief, fuse/protection, cable routing, and legal/electrical safety rules must be handled outside firmware.

## Open hardware validation items

Relevant hardware checks are tracked in `ROADMAP.md`. The most important hardware-facing items are:

- 25 kHz fan PWM on Nano 33 IoT / SAMD21,
- fan PWM signal quality with both fans connected,
- fan tach readings,
- minimum fan PWM percent,
- PWM-to-RPM table,
- required water level sensor mounting height in the real tank,
- real levels for pump warning and stop,
- TCS34725 Clear-channel behavior in the actual room,
- DS3231 32kHz behavior if ever used,
- I2C bus voltage safety,
- relay wiring and mains safety.
