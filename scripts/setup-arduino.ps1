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
arduino-cli core install arduino:samd

arduino-cli lib update-index

$Libraries = @(
    "Adafruit BusIO@1.17.4",
    "Arduino_SpiNINA@0.0.2",
    "PubSubClient@2.8",
    "Sensirion Core@0.7.3",
    "Sensirion I2C SHT4x@1.1.2",
    "RTClib@2.1.4",
    "WiFiNINA@2.0.1",
    "home-assistant-integration@2.1.0",
    "JC_EEPROM@1.0.10",
    "Streaming@6.2.4",
    "NTC_Thermistor@1.1.4",
    "Adafruit TCS34725@1.4.4",
    "IRremote@4.7.1"
)

foreach ($Library in $Libraries) {
    arduino-cli lib install $Library
}

if (Test-Path (Join-Path $RepoRoot "sketch.yaml")) {
    arduino-cli compile --profile nano33iot .
}

arduino-cli version
arduino-cli core list
