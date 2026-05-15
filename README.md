# Arduino Codex Template

Dieses Repository ist für Arduino-Firmware mit Codex vorbereitet.

## Voraussetzungen lokal

- Git for Windows
- Arduino CLI
- Codex-App
- Optional: GitHub Desktop

## Setup

Windows:

    .\scripts\setup-arduino.ps1

Linux / Codex Cloud / GitHub Actions:

    ./scripts/setup-arduino.sh

## Compile-Check

Windows:

    .\scripts\check-arduino.ps1

Linux / Codex Cloud / GitHub Actions:

    ./scripts/check-arduino.sh

## Codex-Workflow

- Neue Features bevorzugt in Codex-Worktrees bearbeiten.
- Erst kompilieren, dann committen.
- Hardwaretests und Uploads lokal durchführen.
- Pull Requests werden durch GitHub Actions kompiliert.

## Empfohlener Ablauf

1. Neuen Thread in Codex als Worktree starten.
2. Codex die gewünschte Änderung umsetzen lassen.
3. Compile-Check ausführen lassen.
4. Diff prüfen.
5. Committen und pushen.
6. Pull Request prüfen lassen.
7. Hardwaretest lokal durchführen.

## Projektstruktur

    .
    ├── AGENTS.md
    ├── README.md
    ├── arduino-cli.yaml
    ├── sketch.yaml
    ├── scripts/
    │   ├── setup-arduino.ps1
    │   ├── setup-arduino.sh
    │   ├── check-arduino.ps1
    │   └── check-arduino.sh
    ├── .github/
    │   └── workflows/
    │       └── arduino-compile.yml
    └── FirmwareName/
        └── FirmwareName.ino

## Hinweise

- `AGENTS.md` enthält die Arbeitsregeln für Codex.
- `scripts/setup-arduino.*` installiert oder vorbereitet Arduino-Cores und benötigte Libraries.
- `scripts/check-arduino.*` kompiliert das Projekt.
- `sketch.yaml` definiert das Arduino-Build-Profil.
- `arduino-cli.yaml` kann zusätzliche Board-Manager-URLs enthalten.
- `.github/workflows/arduino-compile.yml` prüft Pull Requests automatisch.