# DECISIONS.md – AutoTerra design decisions

## Purpose

This document records important AutoTerra design decisions and the reasoning behind them.

It is not a commit changelog. It exists to help future contributors understand why the current design is shaped the way it is.

## Decision format

Decision entries use this structure:

Title line:
`## YYYY-MM-DD – Decision title`

Status:
`Accepted`

Decision:
A short description of the decision.

Reason:
Why this decision was made.

Consequences:
What this means for the project.

## Accepted decisions

## 2026-05-16 – Documentation-first workflow before firmware implementation

Status:
Accepted

Decision:
AutoTerra will complete and review the core documentation set before production firmware implementation starts.

Reason:
The controller combines hardware safety, persistence, Home Assistant integration, fallback behavior, and several sensors. Defining the intended behavior first reduces accidental implementation drift.

Consequences:
Firmware work must follow `SPEC.md`, `MODULES.md`, `entity-model.md`, `libraries.txt`, `HA_DASHBOARD.md`, `HA_AUTOMATIONS.md`, and `AGENTS.md`. Open work and validation tasks belong in `ROADMAP.md`.

## 2026-05-16 – Arduino Nano 33 IoT as the only target board

Status:
Accepted

Decision:
The only target board for v1 is Arduino Nano 33 IoT.

Reason:
The selected hardware, WiFi support, pin plan, and SAMD21 behavior are specific to this board.

Consequences:
No AVR, Arduino Mega, or generic compatibility layer should be added unless the decision is explicitly revisited.

## 2026-05-16 – SHT45 replaces SHT31; no hardware alert mode

Status:
Accepted

Decision:
AutoTerra uses the Sensirion SHT45 for air temperature and humidity, and does not use SHT31-style hardware alert mode.

Reason:
The selected SHT45 design does not provide the earlier SHT31 alert-register workflow.

Consequences:
Threshold evaluation and fault reporting belong in firmware logic, not hardware alert pins or SHT31 alert registers.

## 2026-05-16 – Home Assistant handles configuration and dashboards; Arduino keeps local hardware safety

Status:
Accepted

Decision:
Home Assistant is responsible for configuration, dashboards, notifications, and schedule triggers, while the Arduino remains responsible for local hardware safety and fallback behavior.

Reason:
The controller must remain safe when WiFi, MQTT, or Home Assistant is unavailable.

Consequences:
Pump cutoff, sensor reads, fan PWM generation, RPM measurement, IR output, local fades, and fallback logic must run locally on the Arduino.

## 2026-05-16 – No custom MQTT payload model for v1; use ArduinoHA entities

Status:
Accepted

Decision:
AutoTerra v1 will use ArduinoHA entities instead of a custom MQTT payload architecture.

Reason:
ArduinoHA provides Home Assistant discovery and entity abstractions that match the intended integration model.

Consequences:
Entity design belongs in `entity-model.md`. Fan curves and configuration values may use multiple standard entities rather than custom packed MQTT payloads.

## 2026-05-16 – Text-like status output via sensor entity is accepted

Status:
Accepted

Decision:
Text-like status output through a sensor entity is accepted for v1.

Reason:
A separate proof-of-concept test confirmed that this works in the selected Home Assistant setup.

Consequences:
The firmware may expose system status text through an ArduinoHA sensor entity, and this premise should be preserved in documentation.

## 2026-05-16 – AT24C32 EEPROM is persistent storage; RAM is runtime source of truth

Status:
Accepted

Decision:
The AT24C32 EEPROM stores persistent configuration and recovery data, but RAM is the active source of truth after boot.

Reason:
Constant EEPROM reads are unnecessary and would make runtime behavior more fragile and slower.

Consequences:
The firmware reads EEPROM at boot, copies valid data into RAM, updates RAM immediately on changes, and writes EEPROM only when persistent data changes.

## 2026-05-16 – EEPROM validation uses magic, version, and length, but no CRC

Status:
Accepted

Decision:
EEPROM blobs use minimal validation fields: `magic`, `version`, and `length`. CRC is not used for v1.

Reason:
A CRC stored in the same EEPROM can itself be corrupted, and the extra complexity is not justified for the first version.

Consequences:
The firmware should reject obviously incompatible blobs but should not implement CRC validation unless this decision is revisited.

## 2026-05-16 – Persistent storage uses a config blob and a runtime blob

Status:
Accepted

Decision:
Persistent storage is split into a configuration blob and a runtime recovery blob.

Reason:
Long-lived user configuration and short-lived recovery state have different lifecycles and should not be conflated.

Consequences:
Persistence code must keep configuration values separate from fade recovery, time snapshots, and other runtime recovery data.

## 2026-05-16 – Fan control supports AUTO and MANUAL modes with persistent manual settings

Status:
Accepted

Decision:
Fan control supports AUTO and MANUAL modes. Fan mode and manual fan percent are persistent.

Reason:
Manual fan control must remain predictable during Home Assistant or MQTT loss.

Consequences:
The firmware must retain manual fan mode and the last manual percent across restart and communication loss.

## 2026-05-16 – Fan PWM uses an inverting 2N3904 driver and effective fan percent

Status:
Accepted

Decision:
Internal fan logic uses effective fan percent, while `FanControl` converts that value to the inverted PWM duty required by the external driver circuit. The shared fan PWM signal is driven through an inverting 2N3904 NPN transistor stage. The shared fan PWM node uses a 2.2 kΩ pull-up to +5 V, and the Nano GPIO must not directly drive the two fan PWM inputs. Fan tach pull-ups are separate 10 kΩ resistors to 3.3 V.

Reason:
The transistor stage protects the Nano GPIO from the 5 V fan PWM line, supports both fan PWM inputs from one shared signal, and keeps inversion isolated at the hardware boundary.

Consequences:
`FanControl` owns the conversion from effective fan percent to inverted PWM hardware duty. Tach inputs remain separate and use 3.3 V pull-ups to protect Nano inputs.

## 2026-05-16 – SHT/I²C errors do not automatically force MANUAL fan mode to 100%

Status:
Accepted

Decision:
SHT or I²C errors are reported, but they do not automatically override MANUAL fan mode to 100%.

Reason:
Manual fan mode is an intentional user-controlled state and should not be unexpectedly replaced by fault behavior unless requirements change.

Consequences:
Faults must be visible in Home Assistant, but MANUAL fan mode remains manual during these errors.

## 2026-05-16 – Pump cutoff is local and latched; sensor-failure override is not persistent

Status:
Accepted

Decision:
Pump cutoff is enforced locally and latched. Home Assistant may manually re-enable the pump during water sensor failure, but that override is not persistent.

Reason:
Pump safety must not depend on network availability, and sensor-failure override should require conscious confirmation after every restart.

Consequences:
The firmware must not auto-restart the pump after cutoff and must clear sensor-failure override on Arduino restart.

## 2026-05-16 – Water level downward changes use 6-hour stabilization except without liquid

Status:
Accepted

Decision:
Official downward changes between noncritical water levels require a 6-hour stabilization period, while `without liquid` triggers immediate pump cutoff.

Reason:
Normal water movement may cause transient lower readings, but no-liquid state is safety-critical.

Consequences:
`LiquidSensor` must track candidate lower levels over time and `PumpSafety` must react immediately to `without liquid`.

## 2026-05-16 – Light fallback without valid time uses 8 h User Preset 2 and 16 h off

Status:
Accepted

Decision:
If no valid time source is available beyond the allowed fallback period, light control uses an 8-hour on / 16-hour off timer with User Preset 2.

Reason:
The terrarium should retain a simple, predictable light cycle even without RTC, NTP, or Home Assistant time.

Consequences:
This minimal timer fallback applies to light behavior only and does not replace other safety logic.

## 2026-05-16 – TCS34725 v1 uses global Clear-channel on/off validation only

Status:
Accepted

Decision:
The first TCS34725 implementation uses the Clear raw channel for broad light on/off validation, not full color validation.

Reason:
Room daylight and ceiling light may make full validation unreliable, and the first version only needs a broad expected/actual mismatch check.

Consequences:
Firmware should publish warnings for mismatches, but full RGB/lux/color-temperature validation is deferred.

## 2026-05-16 – Credentials example is deferred until first WiFi/MQTT implementation

Status:
Accepted

Decision:
`Credentials.example.h` will be created with the first WiFi/MQTT implementation, not during documentation-only setup.

Reason:
The exact credential structure should follow the actual WiFi and MQTT implementation interfaces. Until connectivity code exists, credential fields should not be guessed.

Consequences:
Documentation may mention the planned credentials example, but this documentation-only phase does not need to create it yet. The file should contain only credentials that are actually needed by the implemented connectivity layer.

## Superseded decisions

No superseded decisions are recorded yet.
