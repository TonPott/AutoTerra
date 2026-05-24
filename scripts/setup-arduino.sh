#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

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

  mkdir -p "$local_dir" "$arduino_home/data" "$arduino_home/downloads" "$arduino_home/user"

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

mkdir -p "$HOME/.local/bin"

if ! command -v arduino-cli >/dev/null 2>&1; then
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \
    | BINDIR="$HOME/.local/bin" sh
fi

export PATH="$HOME/.local/bin:$PATH"

initialize_arduino_config

arduino-cli core update-index
arduino-cli lib update-index

# Project board
arduino-cli core install arduino:samd

libraries=(
  "Adafruit BusIO@1.17.4"
  "Arduino_SpiNINA@0.0.2"
  "PubSubClient@2.8"
  "Sensirion Core@0.7.3"
  "Sensirion I2C SHT4x@1.1.2"
  "RTClib@2.1.4"
  "WiFiNINA@2.0.1"
  "home-assistant-integration@2.1.0"
  "JC_EEPROM@1.0.10"
  "Streaming@6.2.4"
  "NTC_Thermistor@1.1.4"
  "Adafruit TCS34725@1.4.4"
  "IRremote@4.7.1"
)

for library in "${libraries[@]}"; do
  arduino-cli lib install "$library"
done

if [[ -f "$REPO_ROOT/sketches/AutoTerraController/sketch.yaml" ]]; then
  bash "$REPO_ROOT/scripts/check-arduino.sh"
fi

arduino-cli version
arduino-cli core list
