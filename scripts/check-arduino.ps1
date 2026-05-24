$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

$ToolchainMissingMessage = "Arduino toolchain is not prepared. Run scripts/setup-arduino once, then retry the compile check."

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

    $DataPath = Convert-ToYamlSingleQuotedPath (Join-Path $ArduinoHome "data")
    $DownloadsPath = Convert-ToYamlSingleQuotedPath (Join-Path $ArduinoHome "downloads")
    $UserPath = Convert-ToYamlSingleQuotedPath (Join-Path $ArduinoHome "user")
    $BuildCachePath = Convert-ToYamlSingleQuotedPath (Join-Path $RepoRoot ".arduino-cache")

@"
board_manager:
  additional_urls: []

build_cache:
  path: $BuildCachePath

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

function Assert-ArduinoToolchainPrepared {
    if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
        throw $ToolchainMissingMessage
    }

    $CoreList = arduino-cli core list 2>$null
    if ($LASTEXITCODE -ne 0 -or -not ($CoreList -match "(?m)^arduino:samd\s")) {
        throw $ToolchainMissingMessage
    }

    $RequiredLibraries = @(
        @{ Name = "Adafruit BusIO"; Version = "1.17.4" },
        @{ Name = "Arduino_SpiNINA"; Version = "0.0.2" },
        @{ Name = "PubSubClient"; Version = "2.8" },
        @{ Name = "Sensirion Core"; Version = "0.7.3" },
        @{ Name = "Sensirion I2C SHT4x"; Version = "1.1.2" },
        @{ Name = "RTClib"; Version = "2.1.4" },
        @{ Name = "WiFiNINA"; Version = "2.0.1" },
        @{ Name = "home-assistant-integration"; Version = "2.1.0" },
        @{ Name = "JC_EEPROM"; Version = "1.0.10" },
        @{ Name = "Streaming"; Version = "6.2.4" },
        @{ Name = "NTC_Thermistor"; Version = "1.1.4" },
        @{ Name = "Adafruit TCS34725"; Version = "1.4.4" },
        @{ Name = "IRremote"; Version = "4.7.1" }
    )

    $InstalledLibrariesJson = arduino-cli lib list --json 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw $ToolchainMissingMessage
    }

    $InstalledLibraries = ($InstalledLibrariesJson | ConvertFrom-Json).installed_libraries

    foreach ($Library in $RequiredLibraries) {
        $MatchingLibrary = $InstalledLibraries | Where-Object {
            $_.library.name -eq $Library.Name -and $_.library.version -eq $Library.Version
        } | Select-Object -First 1

        if (-not $MatchingLibrary) {
            throw $ToolchainMissingMessage
        }
    }
}

function Get-SafeSketchName([string] $SketchPath) {
    $FullSketchPath = [System.IO.Path]::GetFullPath($SketchPath)
    $FullRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
    $SketchKey = $FullSketchPath

    if ($FullSketchPath.StartsWith($FullRepoRoot + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
        $SketchKey = $FullSketchPath.Substring($FullRepoRoot.Length + 1)
    }

    $SafeName = $SketchKey -replace '^[A-Za-z]:', ''
    $SafeName = $SafeName -replace '[\\/:\*\?"<>\|\s]+', '_'
    $SafeName = $SafeName.Trim("_")

    if (-not $SafeName) {
        return "sketch"
    }

    return $SafeName
}

Initialize-ArduinoConfig
Assert-ArduinoToolchainPrepared

$DefaultSketch = "sketches/AutoTerraController"
$Sketch = if ($env:SKETCH) { $env:SKETCH } else { $DefaultSketch }
$Fqbn = if ($env:FQBN) { $env:FQBN } else { "arduino:samd:nano_33_iot" }
$Profile = if ($env:PROFILE) { $env:PROFILE } else { "nano33iot" }

$SketchPath = if ([System.IO.Path]::IsPathRooted($Sketch)) {
    $Sketch
} else {
    Join-Path $RepoRoot $Sketch
}

$SafeSketchName = Get-SafeSketchName $SketchPath
$BuildPath = Join-Path (Join-Path $RepoRoot ".build") $SafeSketchName
$BuildCachePath = Join-Path $RepoRoot ".arduino-cache"

New-Item -ItemType Directory -Force -Path $BuildPath | Out-Null
New-Item -ItemType Directory -Force -Path $BuildCachePath | Out-Null

if (Test-Path (Join-Path $SketchPath "sketch.yaml")) {
    arduino-cli compile --profile $Profile --build-path $BuildPath $SketchPath
} else {
    arduino-cli compile --fqbn $Fqbn --build-path $BuildPath $SketchPath
}
