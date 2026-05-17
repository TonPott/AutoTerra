# Automated Terrarium

Arduino Nano 33 IoT firmware project for an automated terrarium controller.

The controller is intended to monitor and control a terrarium with a small water feature. It communicates with Home Assistant over MQTT, while keeping essential hardware safety and fallback behavior local on the Arduino.

## Project Status

This repository is currently in the design and documentation phase.

Production firmware should not be implemented until the documentation set has been reviewed and accepted:

- `SPEC.md`
- `MODULES.md`
- `entity-model.md`
- `libraries.txt`
- `HA_DASHBOARD.md`
- `HA_AUTOMATIONS.md`
- `AGENTS.md`

`AutoTerra.ino` is only a compileable placeholder at this stage.

## Main Goals

The controller shall:

- drive the terrarium light by IR remote commands,
- execute local fades between light presets,
- support manual light control, stand-alone light scheduling, and Home Assistant triggered scheduling,
- measure air temperature and humidity with a Sensirion SHT45,
- measure water temperature with a B3950 10K NTC thermistor,
- measure water level with a frequency-output liquid level sensor,
- control two Noctua 5 V PWM fans,
- monitor both fans by their tachometer outputs,
- locally shut down the pump on critical water level,
- verify simple light on/off state using a TCS34725 light sensor,
- keep time with a DS3231 RTC,
- store persistent configuration in the AT24C32 EEPROM on the RTC module,
- synchronize the RTC once per day from NTP,
- publish sensor values, state, availability, warnings, and faults to Home Assistant.

## Hardware Summary

Target board:

- Arduino Nano 33 IoT

Main external modules:

- DS3231 RTC module with AT24C32 EEPROM
- Sensirion SHT45 temperature/humidity sensor
- B3950 10K NTC thermistor for water temperature
- CQRobot multi-point photoelectric liquid level sensor with frequency output
- CQRobot/Adafruit-compatible TCS34725 light/color sensor
- two Noctua NF-A4x20 5V PWM fans with a shared transistor-buffered PWM line and separate tach inputs
- pump relay module using 3.3 V logic
- IR LED driver circuit for the remote-controlled lamp

## Documentation Map

- `SPEC.md` - complete functional specification
- `MODULES.md` - firmware module responsibilities and interfaces
- `entity-model.md` - Home Assistant entity model
- `HA_DASHBOARD.md` - initial dashboard design ideas
- `HA_AUTOMATIONS.md` - Home Assistant automation responsibilities
- `ROADMAP.md` - open tasks, validation work, risks, and implementation phases
- `DECISIONS.md` - accepted design decisions and rationale
- `AGENTS.md` - Codex working rules for this repository
- `libraries.txt` - required Arduino libraries and their roles

## Important Design Decisions

- The Arduino is the local hardware controller and safety layer.
- Home Assistant is the configuration, dashboard, and automation layer.
- The controller must remain locally safe when WiFi, MQTT, or Home Assistant is unavailable.
- Persistent data is read from EEPROM at boot and copied into RAM. During runtime, RAM is the active source of truth.
- EEPROM is written only when configuration or runtime recovery data changes.
- No custom MQTT payload model is planned for v1. Use ArduinoHA entities.
- Text-like status messages are allowed through a sensor entity because a separate test confirmed this works with the chosen Home Assistant setup.

## Local Setup

Windows:

```powershell
.\scripts\setup-arduino.ps1
```

Linux / Codex Cloud / GitHub Actions:

```bash
./scripts/setup-arduino.sh
```

The setup scripts prepare Arduino CLI, the Nano 33 IoT core, and the libraries listed in `sketch.yaml`.

## Compile Check

Windows:

```powershell
.\scripts\check-arduino.ps1
```

Linux / Codex Cloud / GitHub Actions:

```bash
./scripts/check-arduino.sh
```

The active Arduino profile is `nano33iot` and targets `arduino:samd:nano_33_iot`.

## Build Order

1. Finish and review documentation.
2. Create a small proof of concept for uncertain hardware or library behavior if needed.
3. Implement modules one by one.
4. Integrate Home Assistant entities.
5. Test safety and fallback behavior.
6. Tune dashboard and automations after firmware entities are stable.

Hardware tests and uploads are intentionally local tasks. CI and the provided scripts only verify that the firmware compiles.
