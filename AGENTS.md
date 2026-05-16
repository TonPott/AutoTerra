# Codex-Regeln für AutoTerra

Dieses Repository enthält die Arduino-Firmware-Dokumentation und später die Firmware für den automatisierten Terrarium-Controller `AutoTerra`.

## Projektphase

- Das Projekt ist aktuell in der Design- und Dokumentationsphase.
- Vor produktiver Firmware-Implementierung müssen `SPEC.md`, `MODULES.md`, `entity-model.md`, `libraries.txt`, `HA_DASHBOARD.md` und `HA_AUTOMATIONS.md` gelesen und akzeptiert sein.
- `AutoTerra.ino` bleibt bis dahin nur ein minimaler, kompilierbarer Platzhalter.
- Keine Firmware-Logik implementieren, solange die Aufgabe nicht ausdrücklich Implementierung verlangt.

## Standard-Workflow

- Neue Features bevorzugt in einem Codex-Worktree bearbeiten.
- Nicht direkt auf `main` committen.
- Änderungen klein und reviewbar halten.
- Keine generierten Build-Artefakte committen.
- Vorhandene User-Änderungen nicht zurücksetzen.
- Keine Pinbelegung, Board-Auswahl oder Library-Wechsel ändern, außer ausdrücklich angefordert.
- Offene Entscheidungen klar als `TODO` oder `Open Question` markieren.

## Zielplattform

- Zielboard ist ausschließlich Arduino Nano 33 IoT.
- Kein AVR-/Mega-Kompatibilitätslayer hinzufügen, außer das wird ausdrücklich neu entschieden.
- `sketch.yaml` verwendet das Profil `nano33iot` mit `arduino:samd:nano_33_iot`.

## Firmware-Architektur

- `AutoTerra.ino` darf nur orchestrieren: Module initialisieren, `begin()` aufrufen und periodische `update()`-Methoden aufrufen.
- Keine Business-Logik, Sensorlogik, EEPROM-Zugriffe oder Home-Assistant-Kommandoverarbeitung direkt in `AutoTerra.ino`.
- Hardwarefunktionen gehören in eigene Module gemäß `MODULES.md`.
- Produktionslogik nicht mit blockierenden `delay()`-Aufrufen bauen, außer eine Library oder ein Hardwareprotokoll benötigt ausdrücklich kurze Wartezeiten.
- Scheduling bevorzugt mit `millis()` umsetzen.
- Debugausgaben hinter einem Compile-Time-Debug-Flag halten und nie persistent speichern.

## Home Assistant / MQTT

- Für v1 `arduino-home-assistant` bzw. die Library `home-assistant-integration` verwenden.
- Keine eigene Custom-MQTT-Payload-Architektur für v1 bauen.
- Home-Assistant-Entitäten gemäß `entity-model.md` anlegen und aktuell halten.
- Nach MQTT-/HA-Reconnect aktuelle Zustände, Availability, Warnungen und Faults erneut publizieren.
- Hardware-Faults nicht nur deshalb löschen, weil Home Assistant wieder verbunden ist.
- Viele `Number`-Entitäten für Fan-Kurvenpunkte sind akzeptiert.
- Textartige Statusausgaben über Sensor-Entities sind erlaubt, weil ein separater Test diese Nutzung bestätigt hat.

## Persistenz

- AT24C32 über `JC_EEPROM` verwenden.
- EEPROM beim Boot lesen, danach RAM als aktive Laufzeit-Wahrheit verwenden.
- EEPROM nur schreiben, wenn Konfiguration oder Recovery-Daten geändert wurden.
- Nicht ständig EEPROM während des Betriebs lesen.
- Persistenz in Config-Blob und Runtime-Blob trennen.
- Header-Felder `magic`, `version` und `length` verwenden.
- Für v1 keine CRC implementieren.
- Pumpen-Override bei Wasserstandssensorfehler darf nicht persistent sein.

## Sicherheitsregeln

- Pumpensicherheit bleibt lokal auf dem Arduino.
- Kritischer Wasserstand und Wasserstandssensorfehler schalten die Pumpe lokal ab.
- Pumpe nach Cutoff nicht automatisch neu starten.
- Home Assistant darf bei Sensorfehler bewusst wieder freigeben, aber dieser Override gilt nur bis zum nächsten Arduino-Neustart.
- Firmware darf nur das Relais kommandieren; sichere Netzspannungsverdrahtung, Isolation, Gehäuse und Zugentlastung sind Hardwareanforderungen.
- Nicht annehmen, dass ein GPIO direkt eine Relaisspule treiben kann.

## Bereichsspezifische Regeln

- SHT45 verwenden; keine SHT31-Alert-Mode-Logik reintroduzieren.
- Keine NeoPixel-Alert-Ausgabe reintroduzieren.
- IR-Ausgabe bleibt auf D3, solange die Pinbelegung nicht ausdrücklich geändert wird.
- DS3231 INT/SQW ist für D2 vorgesehen.
- Der Wasserstandssensor ist ein Frequenzausgang, nicht I²C; D5 ist der vorgesehene Eingang.
- Fan-PWM-Treiber ist invertierend; interne Logik arbeitet immer mit effektivem Fan-Prozent.
- Tach-Eingänge sind getrennt je Lüfter und verwenden externe 10-kΩ-Pull-ups.

## Dokumentationsregeln

- Begriffe konsistent mit `SPEC.md` halten.
- `MODULES.md` aktualisieren, wenn sich Modulverantwortungen ändern.
- `entity-model.md` aktualisieren, wenn sich HA-Entitäten ändern.
- `libraries.txt` und `sketch.yaml` gemeinsam aktualisieren, wenn sich Abhängigkeiten ändern.
- `HA_DASHBOARD.md` und `HA_AUTOMATIONS.md` erst finalisieren, wenn Firmware-Entities stabil sind.

## Build / Verify

Nach Code- oder Build-Konfigurationsänderungen muss Codex den passenden Compile-Check ausführen.

Windows / lokale Codex-App:

```powershell
.\scripts\check-arduino.ps1
```

Linux / Codex Cloud / GitHub Actions:

```bash
./scripts/check-arduino.sh
```

Bei reinen Dokumentationsänderungen reicht Diff-Prüfung; falls Build-Dateien oder Sketch-Dateien betroffen sind, kompilieren.

## Commit-Regeln

Codex darf erst committen, wenn:

- der notwendige Arduino-Compile-Check erfolgreich war,
- der Diff geprüft wurde,
- keine unnötigen Dateien geändert wurden,
- keine Build-Artefakte im Commit enthalten sind.

## Done Means

Eine Aufgabe gilt erst als fertig, wenn:

- die angeforderten Dateien aktualisiert sind,
- die Firmware kompiliert, sofern Code oder Build-Konfiguration geändert wurde,
- Codex die geänderten Dateien zusammenfasst,
- offene Risiken oder nicht lokal/hardwareseitig getestete Punkte genannt werden.
