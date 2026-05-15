#!/usr/bin/env bash
set -euo pipefail

FQBN="${arduino:samd:nano_33_iot}"
SKETCH="${SKETCH:-.}"

arduino-cli compile --fqbn "$FQBN" "$SKETCH"