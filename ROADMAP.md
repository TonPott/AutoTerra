# ROADMAP.md – AutoTerra implementation roadmap

## Purpose

This document tracks open tasks, validation work, implementation phases, known risks, and deferred decisions for AutoTerra.

It does not redefine current requirements. The target behavior remains documented in `SPEC.md`, `MODULES.md`, `entity-model.md`, `HA_DASHBOARD.md`, `HA_AUTOMATIONS.md`, and `libraries.txt`.

## Documentation phase

- Keep `SPEC.md`, `MODULES.md`, and `entity-model.md` as current target-state documents.
- Keep `AGENTS.md` as Codex working rules.
- Keep `README.md` as the project overview and documentation map.
- Keep `DECISIONS.md` as the accepted design rationale and design history.
- Avoid scattering TODOs inside requirement documents when a task belongs in `ROADMAP.md`.
- Move validation tasks, open implementation work, known risks, and deferred decisions into this document unless the uncertainty is itself part of the requirement.

## Pre-implementation validation tasks

- Verify ArduinoHA text-like sensor behavior is already completed successfully, but keep the result documented as an accepted premise.
- Verify how to generate approximately 25 kHz PWM on Arduino Nano 33 IoT / SAMD21 for Noctua PWM fans.
- Confirm that fan PWM driver inversion is handled only inside `FanControl`.
- Verify final D6 timer setup and measured PWM frequency on real hardware.
- Verify clean fan PWM signal shape with both fans connected to the shared 2N3904 collector node.
- Validate that the selected ArduinoHA entity types work as expected for fan, select, number, binary sensor, button, switch, and text-like sensor output.
- Validate DS3231 alarm support in RTClib; if incomplete, plan direct register handling inside `RtcClock`.
- Confirm the final pin map against the real hardware layout before firmware implementation.
- Verify whether normal light verification should run with the TCS34725 LED off, on, or in a controlled measurement sequence.
- Verify TCS34725 INT behavior on D7 if future threshold or interrupt use is desired.
- Verify DS3231 32kHz output behavior on D8 before using it for timing or diagnostics.
- Confirm that no connected module exposes Nano 33 IoT pins to 5 V logic levels.
- Revisit `NTC_Thermistor` version selection when version 2.1.1 or later is available through Arduino Library Manager.
- Create `Credentials.example.h` together with the first WiFi/MQTT implementation.
- Ensure `Credentials.example.h` contains only credentials that are actually needed by the implemented connectivity layer.
- Do not guess credential fields before connectivity code exists.

## Firmware implementation phases

- Phase 1: compileable project skeleton.
- Phase 2: infrastructure modules: Debug, Config, Types, TimeSync, Persistence.
- Phase 3: sensor modules: SHT45, NTC, LiquidSensor, TCS34725.
- Phase 4: actuator modules: IRControl, Fade, LightRuntime, FanControl, PumpSafety.
- Phase 5: HomeAssistantBridge entities and command handling.
- Phase 6: fault handling, fallback behavior, reconnect republishing.
- Phase 7: integration testing and dashboard/automation refinement.

## Hardware validation tasks

- Manual hardware validation sketches and test documentation live under [`hardware-tests/`](hardware-tests/README.md).
- Measure minimum reliable PWM percentage for the Noctua fans.
- Provide and later integrate the final PWM-to-RPM calibration table.
- Test fan tach readings with separate external 10 kΩ pull-ups to 3.3 V.
- Test the water level sensor frequencies in the real tank.
- Evaluate the 6-hour stabilization rule with the real water surface behavior.
- Test the immediate pump cutoff at the `without liquid` level.
- Test the TCS34725 Clear-channel light on/off detection in the actual room.
- Test NTC readings with the chosen 8.2 kΩ reference resistor voltage divider.
- Evaluate analog reference strategy only if NTC readings show unacceptable noise or drift.
- Confirm relay module wiring and mains safety outside firmware scope.

## Home Assistant validation tasks

- Validate MQTT discovery behavior for all planned ArduinoHA entities.
- Validate fan, select, number, binary sensor, button, and switch entities in the target Home Assistant installation.
- Validate text-like status output through a sensor entity remains reliable after integration.
- Validate state republishing after MQTT reconnect.
- Validate that module availability and fault entities behave clearly in the dashboard.
- Validate the pump sensor-failure warning near the pump control before allowing manual override workflows.
- Defer final Lovelace dashboard and automation YAML until entity names and types are stable.

## Known technical risks

- Nano 33 IoT PWM frequency may require low-level timer configuration.
- TCS34725 on/off detection may be unreliable due to daylight and room lighting.
- Water level signal may fluctuate because of pump-induced waves.
- EEPROM is not continuously read during runtime; RAM is the source of truth after boot.
- `NTC_Thermistor` version selection may need revisiting.
- RTClib alarm support may be incomplete for the intended DS3231 use.
- ArduinoHA entity behavior may differ between simple proof-of-concept tests and the fully integrated device.
- Pump safety depends on correct external relay wiring, enclosure, isolation, and mains handling, which firmware cannot guarantee.

## Deferred decisions

- Exact SAMD21 timer configuration for approximately 25 kHz fan PWM.
- Final minimum PWM value after fan hardware testing.
- Final PWM-to-RPM calibration table for both fans.
- Exact fallback behavior for AUTO fan mode when SHT45 is unavailable.
- Exact Home Assistant UI layout after entities are implemented and visible.
- Exact RTC alarm implementation path if RTClib alarm support is insufficient.
- Final credentials example structure when WiFi and MQTT code is introduced.

## Out of scope for v1

- Custom MQTT payload architecture.
- Full color validation with TCS34725.
- Automatic fan restart attempts.
- NeoPixel alert display.
- AVR or Arduino Mega compatibility.
- Persisting one-off manual light commands.
- Persisting pump sensor-failure override.
- EEPROM CRC validation.
