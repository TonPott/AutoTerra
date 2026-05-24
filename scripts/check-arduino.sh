#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_SKETCH="sketches/AutoTerraController"
TOOLCHAIN_MISSING_MESSAGE="Arduino toolchain is not prepared. Run scripts/setup-arduino once, then retry the compile check."

autoterra_arduino_home() {
  if [[ -n "${AUTOTERRA_ARDUINO_HOME:-}" ]]; then
    case "$AUTOTERRA_ARDUINO_HOME" in
      /*) printf '%s\n' "$AUTOTERRA_ARDUINO_HOME" ;;
      *) printf '%s\n' "$(pwd)/$AUTOTERRA_ARDUINO_HOME" ;;
    esac
  else
    printf '%s\n' "$HOME/.cache/autoterra/arduino-cli"
  fi
}

yaml_single_quote() {
  printf "'%s'" "$(printf '%s' "$1" | sed "s/'/''/g")"
}

initialize_arduino_config() {
  if [[ -n "${ARDUINO_CONFIG_FILE:-}" ]]; then
    echo "Using user-provided ARDUINO_CONFIG_FILE=$ARDUINO_CONFIG_FILE"
    return
  fi

  local arduino_home local_dir config_file
  arduino_home="$(autoterra_arduino_home)"
  local_dir="$REPO_ROOT/.local"
  config_file="$local_dir/arduino-cli.yaml"

  mkdir -p "$local_dir"

  cat >"$config_file" <<EOF
board_manager:
  additional_urls: []

directories:
  data: $(yaml_single_quote "$arduino_home/data")
  downloads: $(yaml_single_quote "$arduino_home/downloads")
  user: $(yaml_single_quote "$arduino_home/user")

library:
  enable_unsafe_install: false

logging:
  level: info
  format: text
EOF

  export ARDUINO_CONFIG_FILE="$config_file"
  echo "Using shared Arduino CLI home: $arduino_home"
  echo "Generated Arduino CLI config: $config_file"
}

assert_arduino_toolchain_prepared() {
  if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "$TOOLCHAIN_MISSING_MESSAGE" >&2
    exit 1
  fi

  if ! arduino-cli core list 2>/dev/null | grep -Eq '^arduino:samd[[:space:]]'; then
    echo "$TOOLCHAIN_MISSING_MESSAGE" >&2
    exit 1
  fi

  local required_libraries=(
    "Adafruit BusIO|1.17.4"
    "Arduino_SpiNINA|0.0.2"
    "PubSubClient|2.8"
    "Sensirion Core|0.7.3"
    "Sensirion I2C SHT4x|1.1.2"
    "RTClib|2.1.4"
    "WiFiNINA|2.0.1"
    "home-assistant-integration|2.1.0"
    "JC_EEPROM|1.0.10"
    "Streaming|6.2.4"
    "NTC_Thermistor|1.1.4"
    "Adafruit TCS34725|1.4.4"
    "IRremote|4.7.1"
  )

  local installed_libraries_json
  installed_libraries_json="$(arduino-cli lib list --json 2>/dev/null)" || {
    echo "$TOOLCHAIN_MISSING_MESSAGE" >&2
    exit 1
  }

  local entry name version
  for entry in "${required_libraries[@]}"; do
    name="${entry%%|*}"
    version="${entry##*|}"
    if ! printf '%s\n' "$installed_libraries_json" | awk -v expected_name="$name" -v expected_version="$version" '
      /"name":/ {
        current_name = $0
        sub(/^[[:space:]]*"name":[[:space:]]*"/, "", current_name)
        sub(/",[[:space:]]*$/, "", current_name)
      }
      /"version":/ {
        current_version = $0
        sub(/^[[:space:]]*"version":[[:space:]]*"/, "", current_version)
        sub(/",[[:space:]]*$/, "", current_version)
        if (current_name == expected_name && current_version == expected_version) {
          found = 1
        }
      }
      END { exit found ? 0 : 1 }
    '; then
      echo "$TOOLCHAIN_MISSING_MESSAGE" >&2
      exit 1
    fi
  done
}

safe_sketch_name() {
  local sketch_path full_repo full_sketch sketch_key safe_name
  sketch_path="$1"
  full_repo="$(cd "$REPO_ROOT" && pwd -P)"
  full_sketch="$(cd "$(dirname "$sketch_path")" && pwd -P)/$(basename "$sketch_path")"
  sketch_key="$full_sketch"

  case "$full_sketch" in
    "$full_repo"/*) sketch_key="${full_sketch#"$full_repo/"}" ;;
  esac

  safe_name="$(printf '%s' "$sketch_key" | sed -E 's#^[A-Za-z]:##; s#[/\\:*?"<>|[:space:]]+#_#g; s#^_+##; s#_+$##')"
  if [[ -z "$safe_name" ]]; then
    safe_name="sketch"
  fi

  printf '%s\n' "$safe_name"
}

cd "$REPO_ROOT"
initialize_arduino_config
assert_arduino_toolchain_prepared

FQBN="${FQBN:-arduino:samd:nano_33_iot}"
SKETCH="${SKETCH:-$DEFAULT_SKETCH}"
PROFILE="${PROFILE:-nano33iot}"

case "$SKETCH" in
  /*) SKETCH_PATH="$SKETCH" ;;
  *) SKETCH_PATH="$REPO_ROOT/$SKETCH" ;;
esac

SAFE_SKETCH_NAME="$(safe_sketch_name "$SKETCH_PATH")"
BUILD_PATH="$REPO_ROOT/.build/$SAFE_SKETCH_NAME"
BUILD_CACHE_PATH="$REPO_ROOT/.arduino-cache"

mkdir -p "$BUILD_PATH" "$BUILD_CACHE_PATH"

if [[ -f "$SKETCH_PATH/sketch.yaml" ]]; then
  arduino-cli compile --profile "$PROFILE" --build-path "$BUILD_PATH" --build-cache-path "$BUILD_CACHE_PATH" "$SKETCH_PATH"
else
  arduino-cli compile --fqbn "$FQBN" --build-path "$BUILD_PATH" --build-cache-path "$BUILD_CACHE_PATH" "$SKETCH_PATH"
fi
