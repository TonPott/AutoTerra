# HA_DASHBOARD.md – Initial Home Assistant dashboard plan

This document describes a first dashboard concept. The final Lovelace dashboard should be written only after firmware entities are available and tested.

## 1. Dashboard goals

The dashboard shall make it easy to:

- see current environment values,
- control light mode,
- manually control light if needed,
- configure stand-alone schedule,
- configure fan mode and fan curves,
- see pump safety status,
- calibrate water level and light sensor,
- see faults and warnings.

## 2. Suggested dashboard sections

### 2.1 Overview

Show:

- air temperature,
- humidity,
- water temperature,
- water level in mm,
- current light mode,
- current fan mode,
- pump status,
- active faults/warnings.

### 2.2 Light control

Controls:

- `light_mode` select:
  - MANUAL
  - STANDALONE
  - HA_SCHEDULE

Manual control:

- buttons representing remote-control IR commands,
- preset select or preset buttons,
- optional current preset display.

Stand-alone schedule:

- day time,
- night time,
- day preset,
- night preset,
- fade duration.

HA schedule mode:

- dashboard may not directly configure everything in v1,
- HA automations or a schedule card can trigger target preset and fade duration.

### 2.3 Fan control

Show:

- shared fan entity,
- fan mode select,
- manual percent number,
- effective fan PWM percent,
- fan 1 rpm,
- fan 2 rpm,
- fan faults.

Fan curves:

- v1 can expose points as number entities.
- A nicer graph editor can be considered later.

Temperature curve default:

```text
26 °C -> 0%
28 °C -> 20%
30 °C -> 35%
33 °C -> 50%
```

Humidity curve default:

```text
50 %RH -> 0%
65 %RH -> 20%
75 %RH -> 30%
90 %RH -> 50%
```

### 2.4 Water and pump

Show:

- water level mm,
- water temperature,
- pump switch,
- pump cutoff active,
- water level sensor fault,
- water level warning,
- fast drop warning.

Important UI requirement:

If the water level sensor is unavailable, show a visible warning next to the pump switch. The pump may be manually re-enabled, but this is a conscious override and is not persistent across Arduino restarts.

### 2.5 Calibration

Controls:

- store current light clear value as off reference,
- store current light clear value as minimum-on reference,
- set allowed light deviation percent,
- set water level sensor top-to-tank-bottom distance,
- manual RTC/NTP sync button.

### 2.6 Diagnostics

Show:

- module availability,
- I²C health,
- RTC available,
- EEPROM available,
- SHT45 fault,
- TCS fault,
- NTC fault,
- fan faults,
- pump cutoff,
- connection drop counter,
- last time sync source,
- last time sync age,
- system status text.

## 3. Warning style

Warnings should be visible but not necessarily block controls.

Faults should be grouped into a clear "System Health" section.

Suggested distinction:

- red: active local safety/fault condition,
- yellow: warning or unreliable measurement,
- grey: unavailable module.

## 4. Deferred dashboard work

Do not build the final dashboard before firmware entities are available.

First priority:

1. firmware publishes all entities,
2. entity names and types are stable,
3. dashboard design follows those actual entities.
