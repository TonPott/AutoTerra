#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

mkdir -p "$HOME/.local/bin"

if ! command -v arduino-cli >/dev/null 2>&1; then
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \
    | BINDIR="$HOME/.local/bin" sh
fi

export PATH="$HOME/.local/bin:$PATH"
echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"

if [[ -f "$REPO_ROOT/arduino-cli.yaml" ]]; then
  export ARDUINO_CONFIG_FILE="$REPO_ROOT/arduino-cli.yaml"
  echo "export ARDUINO_CONFIG_FILE=\"$REPO_ROOT/arduino-cli.yaml\"" >> "$HOME/.bashrc"
fi

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

if [[ -f "$REPO_ROOT/sketch.yaml" ]]; then
  arduino-cli compile --profile nano33iot .
fi

arduino-cli version
arduino-cli core list
