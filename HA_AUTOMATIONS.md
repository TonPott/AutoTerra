# HA_AUTOMATIONS.md – Planned Home Assistant automations

This document lists automations that should live in Home Assistant rather than the Arduino firmware.

The Arduino remains responsible for hardware safety and local fallback.

## 1. HA light schedule

Home Assistant triggers scheduled light transitions in HA_SCHEDULE mode.

Each trigger sends to Arduino:

- target preset,
- fade duration,
- trigger timestamp.

The Arduino then performs the fade locally.

Possible HA implementations:

- Schedule Card,
- automations based on time helpers,
- scripts that send preset and duration values.

## 2. Water level calibration

Home Assistant shall provide a workflow to set the distance from the water level sensor top edge to the tank bottom.

The Arduino stores this value persistently and uses it to calculate water level in mm.

Possible automation:

1. user enters measured distance,
2. value is sent to Arduino,
3. Arduino stores it in RAM and EEPROM,
4. Arduino republishes calculated water level.

## 3. Light sensor calibration

Home Assistant shall provide buttons:

- save current Clear raw value as light-off reference,
- save current Clear raw value as minimum-on reference.

The Arduino performs the measurement and stores the reference values.

## 4. Time sync automation

Home Assistant dashboard exposes a button:

- manual RTC/NTP synchronization.

Arduino also performs daily NTP sync on its own.

## 5. Warning notifications

Home Assistant may create notifications for:

- water level one level before critical,
- fast water drop warning,
- water temperature out of desired limits,
- fan RPM mismatch warning,
- light expected/actual mismatch,
- pump cutoff active,
- module faults.

Hardware faults should not be cleared merely because Home Assistant reconnects.

## 6. Connection monitoring

Home Assistant can monitor the controller's availability and connection drop counter.

Arduino also tracks failed/lost MQTT/HA sessions and publishes a counter.

## 7. What should not be HA-only

These functions must remain local on Arduino:

- pump cutoff on critical level,
- pump off on water level sensor failure,
- IR command generation,
- local fade calculation,
- fan PWM generation,
- fan RPM measurement,
- sensor reads,
- fallback light mode.

## 8. Deferred automations

Do not create final automations until:

- firmware entity names are stable,
- all entities are visible in Home Assistant,
- basic sensor and control behavior has been tested.
