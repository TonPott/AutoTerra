$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

$ConfigFile = Join-Path $RepoRoot "arduino-cli.yaml"
if (Test-Path $ConfigFile) {
    $env:ARDUINO_CONFIG_FILE = $ConfigFile
}

if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
    throw "arduino-cli wurde nicht gefunden. Bitte Arduino CLI lokal installieren und sicherstellen, dass es im PATH liegt."
}

arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli core install arduino:samd

arduino-cli version
arduino-cli core list