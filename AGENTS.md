# Codex-Regeln für dieses Arduino-Repository

## Projekt
Dieses Repository enthält Arduino-Firmware.

## Standard-Workflow
- Neue Features bevorzugt in einem Codex-Worktree bearbeiten.
- Nicht direkt auf `main` committen.
- Änderungen klein und reviewbar halten.
- Keine generierten Build-Artefakte committen.
- Keine Pinbelegung, Board-Auswahl oder Library-Wechsel ändern, außer ausdrücklich angefordert.

## Build / Verify
Nach Codeänderungen muss Codex den passenden Compile-Check ausführen.

Windows / lokale Codex-App:

```powershell
.\scripts\check-arduino.ps1
```

Linux / Codex Cloud / GitHub Actions:

```bash
./scripts/check-arduino.sh
```

## Commit-Regeln
Codex darf erst committen, wenn:
- der Arduino-Compile-Check erfolgreich war,
- der Diff geprüft wurde,
- keine unnötigen Dateien geändert wurden,
- keine Build-Artefakte im Commit enthalten sind.

## Done means
Eine Aufgabe gilt erst als fertig, wenn:
- die Firmware kompiliert,
- Codex die geänderten Dateien zusammenfasst,
- offene Risiken oder nicht lokal/hardwareseitig getestete Punkte genannt werden.