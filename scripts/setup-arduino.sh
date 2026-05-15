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

# Cores für deine Standardboards
arduino-cli core install arduino:samd
arduino-cli core install arduino:avr

# Wenn sketch.yaml existiert, Profile vorbereiten.
if [[ -f "$REPO_ROOT/sketch.yaml" ]]; then
  arduino-cli compile --profile nano33iot .
  arduino-cli compile --profile mega2560 .
fi

arduino-cli version
arduino-cli core list