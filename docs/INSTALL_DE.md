# KH2 Asset Size Cache – Installation und Rollback

Das eigenständige Windows-x64-Add-on optimiert wiederholte Größenabfragen für
Spieldateien ohne Liste bestimmter Gegner, Dateinamen oder Erweiterungen.
Es ist auch für die Ausführung des x64-Spiels unter Proton auf ARM64 gedacht.
Ein Spieltest unter Proton ARM64 bestätigt deutlich bessere Performance.
Tatsächliche Ladepausen können weiterhin auftreten.

**Testbuild v1-d1:** Die zusätzlichen Messungen und der gezielte Testablauf sind
in [DIAGNOSE_DE.md](DIAGNOSE_DE.md) beschrieben. Die bestätigte Verbesserung
bezieht sich auf den bisherigen Cache v1; der neue Diagnosebuild benötigt
noch einen Spieltest. `DIAG DISABLED` betrifft nur die Zusatzdiagnose.

## Installation über OpenKH

1. Spiel vollständig beenden.
2. Den bisherigen **KH2 Riku Performance Fix** in OpenKH deaktivieren/entfernen
   und die Mods neu bauen. Eine übrig gebliebene `KH2RikuSizeFix.dll` aus
   `mod/kh2/dll` entfernen. Das Backup außerhalb aller Mod-Ladeordner ablegen.
3. `BeZide93/kh2-asset-size-cache` über die GitHub-Installation von OpenKH
   hinzufügen. Alternativ die ZIP aus `packages` als lokalen Mod installieren.
4. KH2 als PC-Spiel auswählen, Mod aktivieren und Mods bauen/anwenden.
5. Unter `mod/kh2/dll` muss `KH2AssetSizeCache.dll` liegen. Nur eine Fassung
   dieses Add-ons und keinen zusätzlichen Riku-Größen-Cache dort laden.
6. Die bestehende Panacea-`version.dll` beibehalten und das Spiel neu starten.

Die ZIP enthält die fertige DLL und den vollständigen Projektinhalt für das
spätere Repository. Quellcode und CI-Dateien müssen zum Spielen nicht gebaut werden.

## Manueller Einbau zum Testen

Nach dem Entfernen des Riku-Fixes `KH2AssetSizeCache.dll` aus der ZIP nach
`mod/kh2/dll` kopieren. Das Spiel muss dabei beendet sein. Ein späterer OpenKH-
Mod-Build kann manuell kopierte Dateien entfernen; für dauerhafte Installation
bevorzugt den lokalen ZIP-Mod verwenden. Die DLL niemals in `version.dll` umbenennen.

## Prüfung

`KH2AssetSizeCache.log` entsteht neben der DLL. Die Startzeile enthält
`slots=256`, `TTL=1000 ms`; `DETECTED:` und `INSTALLED:` bestätigen die Aktivierung.
Bei `DISABLED:` die angegebene Ursache prüfen. Unbekannte Panacea-Implementierungen
werden nicht gepatcht. Statistiken werden alle fünf Sekunden geschrieben;
`candidates`, `hits`, `refresh` und `evicted` zeigen die Cache-Nutzung.

Für den Spieltest dieselben Räume mehrfach betreten und dieselben Gegner erneut
spawnen lassen. Auch Session, Begleiterwechsel, Menüs und Raumwechsel prüfen.
Für den Vergleich die neue Cache-Logdatei und bei vorhandener Diagnose-Panacea
`panacea_riku_perf.log` sowie `panacea_hookfix.log` aufbewahren. Die Diagnose-
Panacea ist keine Voraussetzung zum Spielen; sie ist in dieser ZIP nicht enthalten.

## Rollback

1. Spiel beenden.
2. Diesen Mod in OpenKH deaktivieren/entfernen und Mods neu bauen.
3. Eine verbleibende `KH2AssetSizeCache.dll` aus dem DLL-Ausgabeordner entfernen.
4. Falls gewünscht den vorherigen Riku-Fix wieder aktivieren und Mods neu bauen
   bzw. seine gesicherte DLL zurückkopieren.
5. Spiel neu starten. Keine gleichzeitige Installation beider Größen-Caches.

Spielstände, Assets und Panacea selbst werden durch dieses Paket nicht verändert.

## Grenzen

Der Cache hat 256 feste Plätze und eine maximale Gültigkeit von einer Sekunde
pro Eintrag. Dateipfad, Paketmetadaten und Aufrufkontext werden geprüft.
Fehlende Dateien und Fehler werden nicht gespeichert; unsichere Abfragen gehen
über den ursprünglichen Ladepfad. Änderungen externer Dateien ohne passende
Metadatenänderung werden spätestens bei Ablauf des Eintrags erneut abgefragt.

Die erste Abfrage, notwendige Aktualisierungen und tatsächliche Ladevorgänge
bleiben bestehen. Deshalb ist dies kein Versprechen, alle Standbilder zu beseitigen.
