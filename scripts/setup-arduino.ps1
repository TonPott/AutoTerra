$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

function Get-AutoTerraArduinoHome {
    if ($env:AUTOTERRA_ARDUINO_HOME) {
        return [System.IO.Path]::GetFullPath($env:AUTOTERRA_ARDUINO_HOME)
    }

    $LocalAppData = if ($env:LOCALAPPDATA) {
        $env:LOCALAPPDATA
    } else {
        [Environment]::GetFolderPath("LocalApplicationData")
    }

    if (-not $LocalAppData) {
        $LocalAppData = Join-Path $env:USERPROFILE "AppData\Local"
    }

    return (Join-Path $LocalAppData "AutoTerra\arduino-cli")
}

function Convert-ToYamlSingleQuotedPath([string] $Path) {
    return "'" + $Path.Replace("\", "/").Replace("'", "''") + "'"
}

function Initialize-ArduinoConfig {
    if ($env:ARDUINO_CONFIG_FILE) {
        Write-Host "Using user-provided ARDUINO_CONFIG_FILE=$env:ARDUINO_CONFIG_FILE"
        return
    }

    $ArduinoHome = Get-AutoTerraArduinoHome
    $LocalDir = Join-Path $RepoRoot ".local"
    $ConfigFile = Join-Path $LocalDir "arduino-cli.yaml"

    New-Item -ItemType Directory -Force -Path $LocalDir | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $ArduinoHome "data") | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $ArduinoHome "downloads") | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $ArduinoHome "user") | Out-Null

    $DataPath = Convert-ToYamlSingleQuotedPath (Join-Path $ArduinoHome "data")
    $DownloadsPath = Convert-ToYamlSingleQuotedPath (Join-Path $ArduinoHome "downloads")
    $UserPath = Convert-ToYamlSingleQuotedPath (Join-Path $ArduinoHome "user")

    @"
board_manager:
  additional_urls: []

directories:
  data: $DataPath
  downloads: $DownloadsPath
  user: $UserPath

library:
  enable_unsafe_install: false

logging:
  level: info
  format: text
"@ | Set-Content -Path $ConfigFile -Encoding utf8

    $env:ARDUINO_CONFIG_FILE = $ConfigFile
    Write-Host "Using shared Arduino CLI home: $ArduinoHome"
    Write-Host "Generated Arduino CLI config: $ConfigFile"
}

if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
    throw "arduino-cli was not found. Install Arduino CLI and ensure it is available in PATH."
}

Initialize-ArduinoConfig

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

if (Test-Path (Join-Path $RepoRoot "sketches/AutoTerraController/sketch.yaml")) {
    & (Join-Path $PSScriptRoot "check-arduino.ps1")
}

arduino-cli version
arduino-cli core list
