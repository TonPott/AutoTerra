# entity-model.md – Home Assistant entity model

This document defines the initial Home Assistant entity model for the terrarium controller.

The firmware shall use the `arduino-home-assistant` library and MQTT discovery.

## 1. Naming principles

Entity names should be:

- stable,
- lowercase,
- English,
- grouped by function,
- not tied to exact pin numbers.

Suggested device name:

```text
auto_terra
```

Suggested entity prefix:

```text
terrarium_
```

Example:

```text
terrarium_air_temperature
terrarium_fan1_rpm
terrarium_pump_cutoff_active
```

## 2. Sensors

### 2.1 Climate

| Entity | Type | Unit | Direction | Notes |
|---|---|---:|---|---|
| `air_temperature` | Sensor number | °C | Arduino → HA | SHT45 |
| `air_humidity` | Sensor number | % | Arduino → HA | SHT45 |
| `sht45_available` | Binary sensor | - | Arduino → HA | Availability/fault helper |
| `sht45_fault` | Binary sensor | - | Arduino → HA | true on communication/sensor fault |

### 2.2 Water temperature

| Entity | Type | Unit | Direction | Notes |
|---|---|---:|---|---|
| `water_temperature_c` | Sensor number | °C | Arduino → HA | NTC |
| `water_temperature_f` | Sensor number | °F | Arduino → HA | NTC |
| `water_temperature_k` | Sensor number | K | Arduino → HA | NTC |
| `water_temp_fault` | Binary sensor | - | Arduino → HA | ADC/thermistor fault |

### 2.3 Water level

| Entity | Type | Unit | Direction | Notes |
|---|---|---:|---|---|
| `water_level_mm` | Sensor number | mm | Arduino → HA | primary value |
| `water_level_warning` | Binary sensor | - | Arduino → HA | One level before critical; detected locally by Arduino. |
| `water_level_sensor_fault` | Binary sensor | - | Arduino → HA | invalid/unavailable sensor |
| `water_fast_drop_warning` | Binary sensor | - | Arduino → HA | lower level reached before 6h stabilization |

Optional diagnostics during setup, calibration, or troubleshooting:

| Entity | Type | Unit | Direction | Notes |
|---|---|---:|---|---|
| `water_level_raw_level` | Sensor number | - | Arduino → HA | Optional internal level diagnostic |
| `water_level_frequency` | Sensor number | Hz | Arduino → HA | Optional raw frequency diagnostic |

The normal user-facing Home Assistant value is `water_level_mm`. The optional diagnostic values are not required for the regular dashboard unless setup, calibration, or troubleshooting requires them.

### 2.4 Fans

Use one shared fan entity plus separate sensors.

| Entity | Type | Unit | Direction | Notes |
|---|---|---:|---|---|
| `fans` | Fan | % | HA ↔ Arduino | shared control in manual mode |
| `fan_mode` | Select | - | HA ↔ Arduino | AUTO / MANUAL |
| `fan_pwm_percent` | Sensor number | % | Arduino → HA | effective fan output |
| `fan_manual_percent` | Number | % | HA → Arduino | persistent manual setting |
| `fan1_rpm` | Sensor number | rpm | Arduino → HA | tach 1 |
| `fan2_rpm` | Sensor number | rpm | Arduino → HA | tach 2 |
| `fan1_fault` | Binary sensor | - | Arduino → HA | 0 rpm when expected |
| `fan2_fault` | Binary sensor | - | Arduino → HA | 0 rpm when expected |
| `fan_rpm_mismatch_warning` | Binary sensor | - | Arduino → HA | unexpected RPM compared to table |

### 2.5 Light verification

| Entity | Type | Unit | Direction | Notes |
|---|---|---:|---|---|
| `light_clear_raw` | Sensor number | - | Arduino → HA | TCS34725 clear value |
| `light_mismatch_warning` | Binary sensor | - | Arduino → HA | expected/actual mismatch |
| `light_sensor_fault` | Binary sensor | - | Arduino → HA | TCS unavailable |
| `calibrate_light_off` | Button | - | HA → Arduino | store current Clear as off |
| `calibrate_light_min_on` | Button | - | HA → Arduino | store current Clear as minimum-on |
| `light_allowed_deviation` | Number | % | HA → Arduino | default 5% |

### 2.6 RTC / time / connectivity

| Entity | Type | Unit | Direction | Notes |
|---|---|---:|---|---|
| `rtc_available` | Binary sensor | - | Arduino → HA | DS3231 ok |
| `rtc_time_valid` | Binary sensor | - | Arduino → HA | time usable |
| `last_time_sync_source` | Sensor text-like | - | Arduino → HA | NTP / RTC / RAM / TIMER |
| `last_time_sync_age` | Sensor number | s or min | Arduino → HA | age |
| `sync_time_now` | Button | - | HA → Arduino | manual NTP sync |
| `connection_drop_counter` | Sensor number | count | Arduino → HA | failed/lost HA/MQTT sessions |
| `system_status_text` | Sensor text-like | - | Arduino → HA | allowed because test succeeded |

## 3. Light control entities

| Entity | Type | Direction | Notes |
|---|---|---|---|
| `light_mode` | Select | HA ↔ Arduino | MANUAL / STANDALONE / HA_SCHEDULE |
| `current_light_preset` | Sensor or Select | Arduino → HA / HA ↔ Arduino | design later |
| `fade_active` | Binary sensor | Arduino → HA | runtime |
| `fade_progress` | Sensor number | Arduino → HA | optional |
| `manual_ir_command` | Button group / commands | HA → Arduino | map to command table |

### Stand-alone schedule

| Entity | Type | Direction | Notes |
|---|---|---|---|
| `standalone_day_time_hour` | Number | HA → Arduino | or packed UI later |
| `standalone_day_time_minute` | Number | HA → Arduino | |
| `standalone_night_time_hour` | Number | HA → Arduino | |
| `standalone_night_time_minute` | Number | HA → Arduino | |
| `standalone_day_preset` | Select | HA → Arduino | persistent |
| `standalone_night_preset` | Select | HA → Arduino | persistent |
| `standalone_fade_duration` | Number | HA → Arduino | persistent |

### HA schedule trigger

HA sends:

| Entity / action | Type | Direction | Notes |
|---|---|---|---|
| `ha_schedule_target_preset` | Select | HA → Arduino | target |
| `ha_schedule_fade_duration` | Number | HA → Arduino | duration |
| `ha_schedule_trigger_timestamp` | Number or command field | HA → Arduino | timestamp |

The exact HA UI can be finalized after firmware entities are working.

## 4. Fan curve entities

Custom MQTT is not used.

Represent curves using individual `Number` entities.

Temperature curve:

```text
fan_temp_p1_x
fan_temp_p1_y
fan_temp_p2_x
fan_temp_p2_y
fan_temp_p3_x
fan_temp_p3_y
fan_temp_p4_x
fan_temp_p4_y
```

Humidity curve:

```text
fan_hum_p1_x
fan_hum_p1_y
fan_hum_p2_x
fan_hum_p2_y
fan_hum_p3_x
fan_hum_p3_y
fan_hum_p4_x
fan_hum_p4_y
```

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

## 5. Pump entities

Recommended minimal model:

| Entity | Type | Direction | Notes |
|---|---|---|---|
| `pump_enable` | Switch | HA ↔ Arduino | user request / manual release |
| `pump_cutoff_active` | Binary sensor | Arduino → HA | safety cutoff latched |
| `pump_actual_state` | Binary sensor | Arduino → HA | relay commanded state |
| `water_sensor_unavailable_warning` | Binary sensor | Arduino → HA | show next to pump switch |

If the water level sensor is unavailable:

- pump is turned off,
- HA may re-enable through `pump_enable`,
- this override is not persistent,
- after Arduino restart it must be confirmed again.

## 6. Persistent configuration entities

Values set from Home Assistant and stored in EEPROM:

- light mode
- fan mode
- manual fan percent
- stand-alone schedule
- stand-alone presets
- stand-alone fade duration
- fan curves
- minimum PWM
- temperature warning thresholds
- water level calibration
- pump cutoff level
- light sensor calibration values
- user presets

## 7. Availability behavior

When a module is unavailable:

- its sensor values should be marked unavailable if the entity supports it,
- a module-specific fault binary sensor should become active,
- current state should be republished after HA reconnect.

Do not clear hardware faults merely because HA or MQTT reconnects.
