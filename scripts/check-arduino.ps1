$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

$ConfigFile = Join-Path $RepoRoot "arduino-cli.yaml"
if (Test-Path $ConfigFile) {
    $env:ARDUINO_CONFIG_FILE = $ConfigFile
}

$Sketch = if ($env:SKETCH) { $env:SKETCH } else { "." }
$Fqbn = if ($env:FQBN) { $env:FQBN } else { "arduino:samd:nano_33_iot" }
$Profile = if ($env:PROFILE) { $env:PROFILE } else { "nano33iot" }

$SketchPath = if ([System.IO.Path]::IsPathRooted($Sketch)) {
    $Sketch
} else {
    Join-Path $RepoRoot $Sketch
}

if (Test-Path (Join-Path $SketchPath "sketch.yaml")) {
    arduino-cli compile --profile $Profile $SketchPath
} else {
    arduino-cli compile --fqbn $Fqbn $SketchPath
}
