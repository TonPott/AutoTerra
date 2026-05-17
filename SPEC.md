# SPEC.md – Automated Terrarium

## 1. Scope

This document defines the target behavior of the terrarium controller firmware and its Home Assistant integration.

The project is based on an **Arduino Nano 33 IoT**. It shall operate as an autonomous hardware controller while using Home Assistant for configuration, dashboards, warnings, and scheduled events.

The document is written for a reader with beginner-to-intermediate Arduino and Home Assistant knowledge. It intentionally explains hardware roles, software behavior, and fallback cases in detail.

## 2. Core architecture

### 2.1 Normal operation

During normal operation:

- WiFi is connected.
- MQTT is connected.
- Home Assistant is available.
- The Arduino publishes all sensor and state values.
- Home Assistant provides configuration values.
- The Arduino still executes hardware-facing control locally.

At startup the Arduino shall:

1. initialize hardware modules,
2. read persistent configuration from AT24C32 EEPROM,
3. load configuration into RAM,
4. check RTC time,
5. attempt WiFi and MQTT connection,
6. publish current modes, sensor data, availability, and faults to Home Assistant,
7. resume a stored fade if applicable,
8. immediately evaluate pump safety.

### 2.2 Offline and fallback operation

If MQTT/Home Assistant is unavailable for **5 minutes**, the controller enters reduced stand-alone behavior.

In fallback mode:

- light uses the last stored stand-alone schedule,
- fan behavior uses the last stored fan mode and configuration,
- pump safety remains local,
- faults and counters are retained and republished after reconnect,
- configuration changes from Home Assistant are obviously unavailable.

Connectivity faults themselves are mostly handled in Home Assistant, but the Arduino shall maintain a connection drop / failed connection counter.

### 2.3 Timekeeping and time fallback

Primary time sources:

1. Home Assistant / NTP time
2. DS3231 RTC
3. RAM time snapshots plus uptime
4. simple timer fallback

The RTC shall be synchronized from NTP once per day. Home Assistant shall also expose a button to manually trigger synchronization.

Four times per day, the firmware shall store in RAM:

- a valid wall-clock timestamp from RTC or NTP,
- a corresponding runtime timestamp such as `millis()`.

If the RTC later fails, approximate current time can be reconstructed as:

```text
last_wall_clock_timestamp + elapsed_runtime_since_snapshot
```

If no valid RTC/NTP time is available for up to four days, this reconstructed time may be used. After four days without a valid time source, time-dependent light control falls back to a simple 24-hour timer:

```text
8 hours light using User Preset 2
16 hours off
repeat as a 24-hour cycle
```

This minimal time fallback affects light only. Safety functions such as pump cutoff and fan monitoring still run as far as their sensors allow.

## 3. Light system

### 3.1 Hardware

The terrarium lamp is controlled by IR commands that emulate the original remote control.

The existing IR LED uses pin **D3**.

IR command sending shall be non-blocking and buffered. Commands shall be spaced by a safe minimum interval.

### 3.2 Light modes

The light system has three modes:

```text
MANUAL
STANDALONE
HA_SCHEDULE
```

#### MANUAL

- Time schedules are stopped.
- Home Assistant directly controls the light.
- The Home Assistant frontend shall represent all required remote-control buttons.
- Not every button must be its own HA entity. It is acceptable to store IR commands in a named array and trigger array positions from HA buttons.
- Manual commands are not persistent.

If HA connection is lost, the next trigger starts from the current physical light state. If the Arduino restarts during manual mode and HA is unavailable, the controller uses the last applicable stand-alone preset.

#### STANDALONE

- Arduino runs a local two-event light schedule.
- Two times are configured from HA.
- Times are stored persistently in EEPROM.
- Times are also programmed into the DS3231 alarms.
- EEPROM configuration has priority over RTC alarm configuration during reconciliation.
- Day preset, night preset, and fade duration are configurable.
- Fades are calculated locally.
- If EEPROM is unavailable, hardcoded defaults with the same structure are used.

The DS3231 alarms are used for stand-alone mode only.

#### HA_SCHEDULE

Home Assistant sends scheduled light changes to the Arduino.

Each trigger contains:

- target preset,
- fade duration,
- trigger timestamp.

The Arduino:

- stores the command in RAM immediately,
- writes recovery data to EEPROM if available,
- computes and runs the fade locally,
- can resume or finish the fade after a restart.

If no fade was active before a restart, nothing is changed. The lamp is assumed to still be in its last physical state unless a fallback requires otherwise.

### 3.3 Manual override in schedule modes

Manual commands must also be possible in STANDALONE and HA_SCHEDULE.

Rules:

- a manual command interrupts any running fade,
- the manual light state remains until the next scheduled trigger,
- if the light is manually switched off, the next scheduled trigger must be able to switch it on again,
- manual commands are not persistent.

### 3.4 Preset model

Each RGBW channel has 10 steps:

```text
R: 0..10
G: 0..10
B: 0..10
W: 0..10
```

The firmware shall support:

- all factory presets,
- User Presets 1–4,
- dark/off,
- direct remote-control commands.

Some factory presets may be dynamic in the real lamp. For local fade calculation they shall be modeled as static RGBW values.

Required tables:

- static table for factory presets,
- mutable table for user presets,
- IR command table for remote-control simulation.

## 4. Light verification sensor

A TCS34725 light/color sensor is used to verify whether the lamp is broadly on or off.

The AngelTCS34725US / TCS34725FN module is connected through I2C and power, and also uses prepared optional signal pins:

- TCS34725 LED control is wired to Arduino D4,
- D4 shall be configured as `OUTPUT`,
- D4 LOW means sensor LED off,
- D4 HIGH means sensor LED on,
- the default firmware state shall keep the sensor LED off unless a measurement mode explicitly enables it,
- normal light on/off verification shall use a defined LED state, initially LED off,
- TCS34725 INT is wired to Arduino D7 and prepared for optional future use,
- the TCS34725 INT output is open-drain,
- D7 shall be configured as `INPUT_PULLUP`,
- TCS34725 INT is not required for v1 logic; v1 uses periodic and event-triggered measurements.

First firmware version:

- no full color validation,
- use the **Clear raw value** as the first on/off indicator,
- global calibration only,
- two reference values:
  - light off reference,
  - minimum on reference,
- allowed deviation: 5%.

Measurement shall happen:

- periodically,
- at fade start,
- when a target preset is reached,
- on light mode changes.

Home Assistant shall provide buttons to store the current Clear value as the off or minimum-on reference.

A light expected/actual mismatch is detected in the Arduino and reported to Home Assistant.

The room contains daylight and a ceiling light, so the TCS34725 check may be unreliable. In v1, warnings shall still be published so the behavior can be evaluated.

## 5. Climate sensor: SHT45

The SHT45 replaces the earlier SHT31 design.

Important differences:

- no hardware alert pin,
- no SHT31-style alert registers,
- thresholds are evaluated in firmware.

Requirements:

- use Sensirion arduino-i2c-sht4x library,
- SHT45 I²C address will be hardcoded later,
- measure about twice per minute,
- publish values at least once per minute,
- publish air temperature with one decimal place,
- publish humidity,
- detect and report communication or sensor errors if possible,
- mark SHT45-related entities unavailable when the sensor is unavailable.

SHT45 heater usage is not defined for v1. It remains a documented option for later evaluation, e.g. for condensation recovery, self-drying, or diagnostics.

## 6. Fan control

### 6.1 Hardware

Two Noctua NF-A4x20 5V PWM fans are used.

Design:

- one shared PWM output controls both fans,
- two tach inputs are measured separately,
- PWM driver circuit uses an inverting 2N3904 NPN transistor stage,
- tach signals are open collector,
- tach inputs use separate external 10 kΩ pull-ups to 3.3 V.

#### 6.1.1 Fan supply wiring

Both fans are powered directly from the shared 5 V supply:

- fan yellow wires connect to the +5 V supply,
- fan black wires connect to common GND,
- Arduino Nano 33 IoT is powered from the same 5 V supply through VIN.

All grounds must be common: power supply GND, Nano GND, both fan grounds, and the 2N3904 emitter.

Two NF-A4x20 5V PWM fans need about 0.2 A maximum for the fans alone. The power supply must also reserve current for the Nano, sensors, relay module, startup transients, and load peaks.

A local bulk capacitor on the 5 V rail, for example 100 µF near the fan or power distribution point, is recommended as hardware decoupling. This is not a firmware requirement.

#### 6.1.2 Shared fan PWM driver

The shared fan PWM line is driven by a 2N3904 used as an NPN open-collector style driver:

- Nano D6 connects through a 2.2 kΩ base resistor to the 2N3904 base,
- 2N3904 emitter connects to GND,
- 2N3904 collector connects to the shared fan PWM node,
- the blue PWM wires of both fans connect to this shared collector node,
- the shared fan PWM node has an external 2.2 kΩ pull-up to +5 V,
- an optional 10 kΩ base pulldown from base to GND may be used so the transistor stays off during reset.

This circuit intentionally inverts the PWM signal:

- Arduino output high -> transistor on -> fan PWM line low,
- Arduino output low or high-Z during reset -> transistor off -> fan PWM line pulled high.

Therefore `FanControl` must continue to convert effective fan percent to inverted PWM hardware duty.

The external 2.2 kΩ pull-up is intentional so the PWM line has defined edges with both fans connected and the Nano GPIO does not directly drive or sink both fan PWM inputs. The 5 V PWM line exists only at the fan PWM inputs, the pull-up, and the transistor collector. It must not be directly connected to a Nano input or output pin.

#### 6.1.3 Separate RPM/tach wiring

The fan tach signals are wired separately:

- fan 1 green tach wire connects to D9,
- fan 2 green tach wire connects to D10,
- each tach line has its own external 10 kΩ pull-up to 3.3 V,
- tach outputs must not be tied together,
- tach pull-ups must not connect to 5 V.

The tach outputs are open collector. With 3.3 V pull-ups they are compatible with Nano 33 IoT inputs.

RPM calculation should assume two pulses per revolution unless later hardware testing proves otherwise. D9 and D10 must be used as interrupt-capable or otherwise reliably sampled tach inputs according to the final Nano 33 IoT implementation approach.

#### 6.1.4 PWM frequency

Fan PWM shall target the Noctua / 4-pin-PWM typical high-frequency range around 25 kHz. The timer configuration for Nano 33 IoT / SAMD21 must be implemented intentionally; default `analogWrite()` behavior must not be assumed to produce the required fan-control frequency. The implementation must verify the actual PWM frequency during hardware bring-up.

#### 6.1.5 Protection and level notes

- Tach pull-ups go to 3.3 V to protect Nano inputs.
- The fan PWM pull-up goes to 5 V because it is a fan-control line driven only through the transistor collector and not directly connected to a Nano input or output.
- Do not connect fan tach outputs to 5 V pull-ups.
- Do not drive two fan PWM inputs directly from the Nano GPIO in this design.
- Keep common ground between Arduino and fan supply.

The firmware shall operate in effective fan percentage:

```text
0% = off
100% = full fan output
```

The hardware driver converts effective percentage to the inverted raw PWM output.

Example:

```text
effective 0%   -> hardware duty 100%
effective 40%  -> hardware duty 60%
effective 100% -> hardware duty 0%
```

The `FanControl` hardware layer shall account for the inverting driver and expose only effective fan percentage to the rest of the firmware.

### 6.2 Fan modes

Fan control has its own persistent mode:

```text
AUTO
MANUAL
```

#### AUTO

The Arduino computes fan speed locally from two four-point curves:

- air temperature to fan percent,
- air humidity to fan percent.

The higher computed value wins.

Curve behavior:

- below point 1: 0%,
- between points: linear interpolation,
- above point 4: constant at point 4 output.

Default temperature curve:

```text
26 °C -> 0%
28 °C -> 20%
30 °C -> 35%
33 °C -> 50%
```

Default humidity curve:

```text
50 %RH -> 0%
65 %RH -> 20%
75 %RH -> 30%
90 %RH -> 50%
```

#### MANUAL

Home Assistant sets the fan percentage directly.

Manual fan mode requirements:

- the mode is persistent,
- the last manual percent is persistent,
- the mode is retained during HA/MQTT loss,
- sensor faults are reported but do not automatically override manual mode to 100%.

### 6.3 Fan diagnostics

The firmware shall publish:

- shared effective fan PWM percent,
- fan 1 RPM,
- fan 2 RPM,
- fan 1 fault,
- fan 2 fault,
- optional RPM mismatch warning.

A fixed calibration table will be provided later:

```text
effective PWM percent -> expected RPM
```

The table applies to both fans initially and is not editable from Home Assistant.

Minimum PWM starts at 15% and must be evaluated in real hardware.

No automatic fan restart sequence is required.

## 7. Water temperature

A B3950 10K NTC thermistor measures water temperature.

Requirements:

- connected to analog input,
- voltage divider topology,
- 8.2 kΩ reference resistor from 3.3 V to A0,
- B3950 10K NTC from A0 to GND,
- nominal thermistor resistance: 10 kΩ,
- B value: 3950,
- nominal temperature: 25 °C,
- use NTC_Thermistor library,
- publish all available temperature units:
  - Celsius,
  - Fahrenheit,
  - Kelvin.

The frontend later decides which value is displayed.

Wiring:

```text
3.3 V -> 8.2 kΩ -> A0 -> B3950 NTC -> GND
```

The 8.2 kΩ resistor is the upper reference leg, and the NTC is the lower leg to GND.

## 8. Water level

### 8.1 Sensor

The liquid level sensor is **not I²C-based**. It outputs a square wave.

Nominal frequencies:

```text
without liquid: 20 Hz
Level 1:        50 Hz
Level 2:       100 Hz
Level 3:       200 Hz
Level 4:       400 Hz
```

The sensor shall be read using a digital interrupt-capable input with INPUT_PULLUP.

The assigned pin is **D5**, because D2 is used for RTC interrupt and D3 is used for IR.

### 8.2 Measurement and publication

The level is evaluated once per minute.

Normal HA publication:

- water level in mm.

Optional diagnostics during setup, calibration, or troubleshooting:

- internal level,
- raw frequency.

Raw frequency is not required for the normal dashboard and should not be considered part of the regular user-facing entity set unless calibration or troubleshooting requires it.

### 8.3 Mechanical calibration

Home Assistant configures the distance from sensor top edge to tank bottom. This value must be stored in Arduino EEPROM because the Arduino calculates the water height.

Sensor geometry:

```text
total sensor height: 78.2 mm
Level 4 is 24.4 mm below sensor top edge
each lower level is 16 mm below the previous one
```

Pump cutoff is based on **level**, not mm.

### 8.4 Stability rule

Level changes between noncritical levels are debounced over a long period.

Example:

- official level is Level 4,
- Level 3 is measured,
- timestamp is stored,
- if Level 4 is measured again, the Level 3 candidate is discarded,
- if Level 4 is not measured for 6 hours, Level 3 becomes official.

Exception:

- without liquid / 20 Hz triggers pump cutoff immediately.

Leak warning:

- if the next lower level is reached during the 6-hour waiting period, publish a possible leak / fast-loss warning to Home Assistant.

## 9. Pump safety

The relay module is considered Arduino-ready and uses 3.3 V logic.

The pump is controlled locally for safety.

Rules:

- critical water level immediately shuts off pump,
- pump cutoff is latched,
- no automatic restart after cutoff,
- manual release is required,
- when water level sensor is unavailable, pump is turned off,
- Home Assistant may manually re-enable the pump despite sensor unavailability,
- this sensor-failure override is **not persistent**,
- after every Arduino restart, the user must confirm pump enable again while the sensor remains unavailable.

Home Assistant shall show a visible warning next to the pump switch when the water level sensor is unavailable.

Mains voltage safety must be documented separately. The firmware can command the relay, but safe wiring, isolation, enclosure, and strain relief are hardware requirements.

## 10. RTC and EEPROM

### 10.1 DS3231

The DS3231 provides local time and two alarms.

Requirements:

- DS3231 INT/SQW pin uses INPUT_PULLUP,
- assigned pin: D2,
- DS3231 INT/SQW remains on D2 and is used for stand-alone alarm triggers,
- DS3231 32kHz output is wired to Arduino D8 and prepared for optional future use,
- D8 shall be configured as INPUT_PULLUP,
- the 32kHz signal is not used by v1 logic and no interrupt shall be attached by default,
- if 32kHz is enabled later, it must be handled carefully because it is a high-frequency signal,
- alarms are used for stand-alone light mode,
- RTC is synchronized from NTP daily,
- HA exposes a manual sync button.

### 10.2 AT24C32 EEPROM

The AT24C32 is accessed using JC_EEPROM.

Design rule:

```text
EEPROM is persistent storage.
RAM is the active runtime source of truth.
```

At boot:

- read EEPROM,
- validate minimal header,
- copy data to RAM.

At runtime:

- use RAM values,
- update RAM immediately on HA changes,
- write EEPROM only when configuration or recovery data changes.

If EEPROM becomes unavailable during operation:

- report error,
- continue with RAM configuration.

If EEPROM is unavailable at boot:

- use hardcoded defaults,
- accept HA configuration if HA becomes available,
- operate non-persistently until EEPROM recovers.

### 10.3 EEPROM validation

Use a minimal header only:

```text
magic
version
length
```

No CRC.

Reason:

- CRC would be stored in the same EEPROM and may itself be corrupted,
- false invalidation of otherwise valid data is possible,
- implementation complexity is not justified for v1.

Use two logical blobs:

- Config blob,
- Runtime blob.

## 11. Persistence data

### 11.1 Persistent configuration

Store:

- light mode,
- fan mode,
- last manual fan percent,
- stand-alone schedule times,
- stand-alone day/night presets,
- stand-alone fade duration,
- fan curves,
- minimum PWM,
- temperature warning thresholds,
- water level calibration,
- pump cutoff threshold / level,
- light sensor calibration values,
- user presets.

### 11.2 Runtime recovery data

Store:

- whether a fade is active,
- fade start time,
- fade duration,
- fade start state,
- fade target state,
- last relevant light state,
- last valid time snapshot if needed,
- offline fault counters if HA was unreachable.

### 11.3 Not persistent

Do not persist:

- regular sensor measurements,
- debug flag,
- one-off manual light commands,
- pump sensor-failure override,
- transient runtime states not needed for recovery.

## 12. Faults, warnings, diagnostics, and availability

### 12.1 Definitions

A fault is a state that affects local operation or disables a module.

A warning is a state that should be visible but may not require local intervention.

A diagnostic value explains system state.

### 12.2 Self-diagnosis

Each module shall provide a health / availability state.

I²C problems are detected by checking each I²C transaction or module library return value. A central I²C health check may also be added, but individual module checks remain required.

When a module is known to be unavailable, its corresponding HA entities shall be marked unavailable if the chosen entity type supports availability.

### 12.3 Fault examples

Faults:

- SHT45 unavailable,
- RTC unavailable,
- EEPROM unavailable,
- TCS34725 unavailable,
- NTC open/short/unavailable,
- water level sensor unavailable,
- pump cutoff active,
- fan 0 RPM while expected to run.

Warnings:

- light expected/actual mismatch,
- water level one level before critical,
- RPM mismatch,
- water temperature limits,
- fast water-level drop.

Diagnostics:

- last time sync source,
- last time sync age,
- connection drop counter,
- fallback mode,
- module availability.

Hardware fault entities must not be automatically cleared only because HA reconnects. Current fault state must be republished after reconnect.

## 13. Home Assistant role

Home Assistant is responsible for:

- configuration,
- dashboards,
- warnings/notifications,
- HA schedule mode,
- water-level calibration,
- light sensor calibration,
- RTC manual sync button.

Home Assistant automations handle:

- HA light schedule triggers,
- calibration workflows,
- warning notifications,
- frontend display.

The Arduino remains responsible for:

- hardware signal processing,
- pump safety,
- fan PWM generation and RPM measurement,
- light IR output,
- local fades,
- fallback behavior.

No custom MQTT payload model shall be used in v1. Use ArduinoHA entities.

A previous isolated test confirmed that a sensor entity can be used successfully for text-like status output with the selected ArduinoHA setup.

## 14. Suggested pin map

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

This is the current intended pin map. Pin assignments may change only through an explicit design update.

General hardware note:

- all logic signals connected directly to the Nano 33 IoT must be 3.3 V compatible,
- I2C module supply and pull-ups must not expose Nano pins to 5 V unless proper level shifting or isolation is used.

## 15. Documentation-first implementation rule

Before firmware implementation, generate and review:

- `SPEC.md`,
- `MODULES.md`,
- `entity-model.md`,
- `AGENTS.md`,
- `libraries.txt`,
- `HA_DASHBOARD.md`,
- `HA_AUTOMATIONS.md`.

`Credentials.example.h` is created together with the first WiFi/MQTT implementation. It should contain only credentials that are actually needed by the implemented connectivity layer. Until connectivity code exists, credential fields should not be guessed.

Firmware implementation starts only after the documentation is accepted.
