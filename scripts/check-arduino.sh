#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_SKETCH="sketches/AutoTerraController"

if [[ -z "${ARDUINO_CONFIG_FILE:-}" && -f "$REPO_ROOT/arduino-cli.yaml" ]]; then
  export ARDUINO_CONFIG_FILE="$REPO_ROOT/arduino-cli.yaml"
fi

FQBN="${FQBN:-arduino:samd:nano_33_iot}"
SKETCH="${SKETCH:-$DEFAULT_SKETCH}"
PROFILE="${PROFILE:-nano33iot}"

case "$SKETCH" in
  /*) SKETCH_PATH="$SKETCH" ;;
  *) SKETCH_PATH="$REPO_ROOT/$SKETCH" ;;
esac

if [[ -f "$SKETCH_PATH/sketch.yaml" ]]; then
  arduino-cli compile --profile "$PROFILE" "$SKETCH_PATH"
else
  arduino-cli compile --fqbn "$FQBN" "$SKETCH_PATH"
fi
