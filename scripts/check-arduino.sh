#!/usr/bin/env bash
set -euo pipefail

FQBN="${FQBN:-arduino:samd:nano_33_iot}"
SKETCH="${SKETCH:-.}"
PROFILE="${PROFILE:-nano33iot}"

if [[ -f "$SKETCH/sketch.yaml" ]]; then
  arduino-cli compile --profile "$PROFILE" "$SKETCH"
else
  arduino-cli compile --fqbn "$FQBN" "$SKETCH"
fi
