# MODULES.md – Firmware module design

This document describes the intended firmware modules for the Terrarium controller.

The goal is clear module boundaries. `AutoTerra.ino` should coordinate modules but not contain hardware or business logic.

## 1. Module overview

```text
AutoTerra.ino

Config.h / Config.cpp
Types.h
Debug.h
Credentials.h

RtcClock.h / RtcClock.cpp
TimeSync.h / TimeSync.cpp
Persistence.h / Persistence.cpp
ModeManager.h / ModeManager.cpp
FaultManager.h / FaultManager.cpp

IRMap.h / IRMap.cpp
IRControl.h / IRControl.cpp
Fade.h / Fade.cpp
LightRuntime.h / LightRuntime.cpp
LightSensorTcs34725.h / LightSensorTcs34725.cpp

Sht45Sensor.h / Sht45Sensor.cpp
WaterTempNtc.h / WaterTempNtc.cpp
LiquidSensor.h / LiquidSensor.cpp
FanControl.h / FanControl.cpp
PumpSafety.h / PumpSafety.cpp

WiFiManager.h / WiFiManager.cpp
HomeAssistantBridge.h / HomeAssistantBridge.cpp
```

## 2. `AutoTerra.ino`

Responsibilities:

- initialize all modules,
- call `begin()` methods,
- call periodic `update()` methods,
- keep loop non-blocking,
- avoid hardware-specific logic.

`AutoTerra.ino` shall not:

- directly parse Home Assistant commands,
- directly read sensors except for temporary diagnostics,
- directly manipulate EEPROM,
- directly contain fallback state machine logic.

## 3. `Config.*`

Responsibilities:

- pin assignments,
- default intervals,
- default fan curves,
- default light schedule,
- default pump behavior,
- compile-time options,
- non-secret constants.

Do not store WiFi or MQTT passwords here. Use `Credentials.h`.

## 4. `Types.h`

Responsibilities:

- shared enums,
- shared structs,
- small data transfer structures,
- persistent config and runtime structure declarations.

Expected types:

- `LightMode`
- `FanMode`
- `PresetId`
- `RgbwLevel`
- `LightPreset`
- `FanCurvePoint`
- `FanCurve`
- `PersistentConfig`
- `PersistentRuntime`
- `FaultFlags`
- `ModuleAvailability`

## 5. `Debug.h`

Responsibilities:

- provide debug macros,
- make debug compile-time controlled,
- never persist debug state,
- allow module-prefixed debug messages.

Suggested macros:

```cpp
DBG_PRINT(...)
DBG_PRINTLN(...)
DBG_PRINTF(...)
DBG_MODULE(moduleName, message)
```

Debug output is useful for development but cannot be required for normal operation.

## 6. `RtcClock.*`

Responsibilities:

- initialize DS3231,
- read current time,
- configure two DS3231 alarms for stand-alone light mode,
- read and clear alarm flags,
- report RTC availability,
- detect invalid time if possible.

The RTC alarm line uses `INPUT_PULLUP`.

The DS3231 alarms are only for stand-alone mode.

## 7. `TimeSync.*`

Responsibilities:

- synchronize RTC from NTP once per day,
- handle manual sync request from Home Assistant,
- keep RAM time snapshots four times per day,
- provide fallback time from last timestamp plus uptime,
- expose last sync source and sync age.

Time source priority:

1. HA/NTP
2. DS3231 RTC
3. RAM timestamp + `millis()`
4. 8h on / 16h off timer fallback for light only

## 8. `Persistence.*`

Responsibilities:

- manage AT24C32 EEPROM using JC_EEPROM,
- read persistent blobs at boot,
- validate minimal headers,
- copy EEPROM data into RAM config/runtime,
- write EEPROM on changes,
- avoid constant EEPROM reads during runtime.

Persistent storage model:

- Config blob
- Runtime blob

Header:

```text
magic
version
length
```

No CRC.

Runtime rule:

```text
RAM is the active source of truth.
EEPROM is only boot source and persistent backup.
```

## 9. `ModeManager.*`

Responsibilities:

- track active light mode,
- track active fan mode,
- detect HA timeout,
- switch to stand-alone fallback after 5 minutes,
- expose current fallback state,
- coordinate mode changes without duplicating module logic.

Important:

- light mode and fan mode are separate.
- fan manual mode remains active during HA loss.
- manual light commands are not persistent.

## 10. `FaultManager.*`

Responsibilities:

- collect faults from all modules,
- distinguish faults, warnings, and diagnostics,
- expose module availability,
- decide which local reactions are required,
- keep faults latched when appropriate,
- republish current fault state after HA reconnect.

FaultManager shall not hide hardware faults just because communication has recovered.

## 11. `IRMap.*`

Responsibilities:

- store IR address and command codes,
- store remote-button table,
- store factory preset command mapping,
- store user preset command mapping if applicable.

No timing or send logic belongs here.

## 12. `IRControl.*`

Responsibilities:

- initialize IR output,
- queue IR commands,
- send commands non-blockingly,
- enforce minimum command spacing,
- report buffer overflow.

D3 is currently assigned to the IR LED.

## 13. `Fade.*`

Responsibilities:

- compute local fade between RGBW states,
- use RGBW levels 0..10,
- support resume from elapsed time,
- report fade progress and completion,
- request IR commands through `IRControl`.

Fade does not decide why a fade starts. That belongs to LightRuntime or Schedule handling.

## 14. `LightRuntime.*`

Responsibilities:

- implement light modes,
- handle manual commands,
- handle stand-alone triggers,
- handle HA schedule triggers,
- store fade recovery data,
- decide when to use fallback,
- interact with TCS34725 validation.

Manual command behavior:

- interrupts active fade,
- lasts until next scheduled trigger,
- is not persistent.

HA schedule behavior:

- store start time, target preset, duration, start state, target state,
- resume or complete after restart if runtime data is available.

## 15. `LightSensorTcs34725.*`

Responsibilities:

- initialize TCS34725,
- read Clear raw value,
- optionally read RGB, lux, and color temperature,
- perform on/off verification,
- provide current clear value for calibration,
- detect sensor availability.

Calibration values:

- off reference clear,
- minimum-on reference clear,
- global deviation threshold, initially 5%.

Events to measure:

- periodic checks,
- fade start,
- target preset reached,
- light mode change.

## 16. `Sht45Sensor.*`

Responsibilities:

- initialize SHT45,
- read air temperature and humidity,
- publish or provide values at regular intervals,
- report availability,
- expose measurement error state.

No hardware alert logic exists.

Data use:

- values are published to HA,
- values are used locally by FanControl in AUTO mode.

If SHT45 is unavailable:

- publish unavailable/fault,
- AUTO fan behavior must use a defined fallback,
- MANUAL fan mode remains manual unless later explicitly changed.

## 17. `WaterTempNtc.*`

Responsibilities:

- initialize thermistor reading,
- read analog input,
- convert using NTC_Thermistor,
- provide Celsius, Fahrenheit, Kelvin,
- detect implausible values if possible,
- report availability/fault.

A0 is the suggested analog input.

## 18. `LiquidSensor.*`

Responsibilities:

- count square-wave pulses from liquid sensor,
- classify sensor level,
- compute water height in mm,
- apply 6-hour stabilization rule for noncritical level decreases,
- immediately report without-liquid state,
- report fast drop warning,
- report sensor availability.

Normal publication:

- water level in mm.

Optional diagnostics:

- level,
- raw frequency.

PumpSafety consumes raw or classified level for safety decisions.

## 19. `FanControl.*`

Responsibilities:

- implement AUTO and MANUAL fan modes,
- store and apply manual fan percent,
- apply temperature and humidity fan curves,
- compute effective fan percent,
- convert effective percent to inverted PWM hardware duty,
- measure two tach inputs,
- compute RPM for each fan,
- diagnose RPM faults/mismatches,
- expose values to HomeAssistantBridge.

Persistent values:

- fan mode,
- manual percent,
- fan curves,
- minimum PWM.

Important:

- PWM driver is inverting.
- tach pull-ups are external 10 kΩ.
- one RPM table will be provided later for both fans.

## 20. `PumpSafety.*`

Responsibilities:

- control pump relay output,
- enforce critical water level cutoff,
- latch pump cutoff,
- handle sensor failure pump-off rule,
- allow manual HA release,
- ensure sensor-failure release is not persistent,
- expose pump actual/commanded/cutoff states.

Rules:

- without-liquid level triggers immediate cutoff,
- pump does not auto-restart,
- after restart, sensor-failure override must be confirmed again if sensor still fails.

## 21. `WiFiManager.*`

Responsibilities:

- connect to WiFi using WiFiNINA,
- reconnect as needed,
- expose WiFi state,
- avoid storing secrets in source files other than `Credentials.h`.

## 22. `HomeAssistantBridge.*`

Responsibilities:

- create ArduinoHA device,
- define all HA entities,
- publish values,
- receive commands,
- translate HA commands into module calls,
- publish availability,
- republish state after reconnect.

No custom MQTT payload architecture is planned for v1.

A previous test confirmed that a sensor entity can publish text-like status values in the chosen Home Assistant setup.

## 23. Update timing overview

Suggested starting intervals:

| Module | Interval / trigger |
|---|---|
| SHT45 | measure ~30 s, publish >=60 s |
| NTC | ~60 s |
| LiquidSensor | ~60 s |
| TCS34725 | periodic + light events |
| Fan RPM | frequent enough for stable RPM, design later |
| HA publish | on change + periodic heartbeat |
| RTC/NTP sync | daily + manual button |
| RAM time snapshot | four times daily |
| EEPROM write | only on changes |
