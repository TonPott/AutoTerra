# Codex rules for AutoTerra

This repository contains the Arduino firmware documentation and later the firmware for the automated terrarium controller `AutoTerra`.

## Project phase

- The project is currently in the design and documentation phase.
- Before production firmware implementation starts, `SPEC.md`, `MODULES.md`, `entity-model.md`, `libraries.txt`, `HA_DASHBOARD.md`, and `HA_AUTOMATIONS.md` must be read and accepted.
- Until then, `AutoTerra.ino` remains only a minimal compileable placeholder.
- Do not implement firmware logic unless the task explicitly asks for implementation.

## Standard workflow

- Prefer working on new features in a Codex worktree.
- Do not commit directly to `main`.
- Keep changes small and reviewable.
- Do not commit generated build artifacts.
- Do not revert existing user changes.
- Do not change pin assignments, board selection, or libraries unless explicitly requested.
- Mark open decisions clearly as `TODO` or `Open Question`.

## Target platform

- The only target board is Arduino Nano 33 IoT.
- Do not add an AVR or Arduino Mega compatibility layer unless that decision is explicitly revisited.
- `sketch.yaml` uses the `nano33iot` profile with `arduino:samd:nano_33_iot`.

## Firmware architecture

- `AutoTerra.ino` may only orchestrate: initialize modules, call `begin()`, and call periodic `update()` methods.
- Do not put business logic, sensor logic, EEPROM access, or Home Assistant command handling directly in `AutoTerra.ino`.
- Hardware features belong in separate modules according to `MODULES.md`.
- Do not build production logic with blocking `delay()` calls unless a library or hardware protocol explicitly requires a short wait.
- Prefer `millis()`-based scheduling.
- Keep debug output behind a compile-time debug flag and never persist debug state.

## Home Assistant / MQTT

- For v1, use `arduino-home-assistant`, provided by the `home-assistant-integration` library.
- Do not build a custom MQTT payload architecture for v1.
- Create and maintain Home Assistant entities according to `entity-model.md`.
- After MQTT or Home Assistant reconnect, republish current states, availability, warnings, and faults.
- Do not clear hardware faults merely because Home Assistant has reconnected.
- Many `Number` entities for fan curve points are acceptable.
- Text-like status output through sensor entities is allowed because a separate test confirmed this works.

## Persistence

- Use AT24C32 through `JC_EEPROM`.
- Read EEPROM at boot, then use RAM as the active runtime source of truth.
- Write EEPROM only when configuration or recovery data changes.
- Do not constantly read EEPROM during operation.
- Split persistence into a config blob and a runtime blob.
- Use header fields `magic`, `version`, and `length`.
- Do not implement CRC for v1.
- Pump override during water level sensor failure must not be persistent.

## Safety rules

- Pump safety remains local on the Arduino.
- Critical water level and water level sensor failure turn the pump off locally.
- Do not automatically restart the pump after cutoff.
- Home Assistant may consciously re-enable the pump during sensor failure, but this override only lasts until the next Arduino restart.
- Firmware may only command the relay; safe mains wiring, isolation, enclosure, and strain relief are hardware requirements.
- Do not assume that a GPIO can drive a relay coil directly.

## Domain-specific rules

- Use SHT45; do not reintroduce SHT31 alert-mode logic.
- Do not reintroduce NeoPixel alert output.
- IR output remains on D3 unless the pin assignment is explicitly changed.
- DS3231 INT/SQW is intended for D2.
- D4 is reserved for TCS34725 LED control; D4 LOW means TCS34725 LED off and D4 HIGH means LED on.
- The water level sensor is a frequency output, not I²C; D5 is the intended input.
- The fan PWM driver is inverting; internal logic always works with effective fan percent.
- Fan PWM is driven through an inverting 2N3904 transistor stage from D6; the shared fan PWM node uses a 2.2 kΩ pull-up to +5 V and must not be driven directly by the Nano GPIO.
- D7 is reserved for optional TCS34725 INT and shall use `INPUT_PULLUP` because the INT output is open-drain.
- D8 is reserved for optional DS3231 32kHz output and shall use `INPUT_PULLUP`.
- Tach inputs are separate per fan and use external 10 kΩ pull-ups to 3.3 V.
- Do not move D4, D7, or D8 unless explicitly requested.
- Do not attach interrupts to the DS3231 32kHz input in v1.

## Documentation rules

- Keep terminology consistent with `SPEC.md`.
- Update `MODULES.md` when module responsibilities change.
- Update `entity-model.md` when Home Assistant entities change.
- Update `libraries.txt` and `sketch.yaml` together when dependencies change.
- Open tasks, validation work, known risks, and deferred decisions belong in `ROADMAP.md`.
- Accepted design rationale belongs in `DECISIONS.md`.
- Requirement documents should describe the current target state and should not be cluttered with implementation TODOs unless the uncertainty is itself part of the requirement.
- Do not create `Credentials.example.h` during documentation-only work. Create it together with the first WiFi/MQTT implementation, and include only credentials that are actually required by that implementation.
- Finalize `HA_DASHBOARD.md` and `HA_AUTOMATIONS.md` only after firmware entities are stable.

## Build / verify

After code or build configuration changes, Codex must run the matching compile check.

Windows / local Codex app:

```powershell
.\scripts\check-arduino.ps1
```

Linux / Codex Cloud / GitHub Actions:

```bash
./scripts/check-arduino.sh
```

For documentation-only changes, diff review is sufficient. If build files or sketch files changed, compile.

## Commit rules

Codex may commit only after:

- the required Arduino compile check has passed,
- the diff has been reviewed,
- no unnecessary files were changed,
- no build artifacts are included in the commit.

## Done means

A task is complete only when:

- the requested files are updated,
- the firmware compiles when code or build configuration changed,
- Codex summarizes the changed files,
- open risks or points not tested locally or on hardware are named.
