# KH2 Asset Size Cache – Installation und Rollback

Das eigenständige Windows-x64-Add-on optimiert wiederholte Größenabfragen für
Spieldateien ohne Liste bestimmter Gegner, Dateinamen oder Erweiterungen.
Es ist auch für die Ausführung des x64-Spiels unter Proton auf ARM64 gedacht.
Ein Spieltest unter Proton ARM64 bestätigt deutlich bessere Performance.
Tatsächliche Ladepausen können weiterhin auftreten.

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

`KH2AssetSizeCache.log` neben der DLL enthält genau eine Statuszeile:
`INSTALLED:` bestätigt den aktiven Größen-Cache. Zusätzlich muss für diesen Test
`path_cache=ACTIVE` erscheinen. Bei `path_cache=DISABLED (...)` läuft nur der
Größen-Cache; die Klammer nennt den Grund. `DISABLED:` am Anfang nennt den Grund,
warum er nicht aktiviert wurde, beispielsweise eine inkompatible Panacea-DLL.
Bei jedem neuen Spielstart ersetzt die Initialisierung den bisherigen Inhalt.
Während des Spielens werden keine weiteren Zeilen geschrieben und es läuft
kein Statistik-Thread. Ein erneuter `OnInit`-Aufruf im selben Prozess bleibt ohne Wirkung.

Wird die DLL gar nicht geladen oder `OnInit` nicht aufgerufen, kann sie auch
keine Logdatei aktualisieren. Ein alter Eintrag ist dann kein Nachweis für den
aktuellen Start; den Änderungszeitpunkt prüfen. Bei fehlenden Schreibrechten
kann die Statusausgabe ebenfalls ausbleiben, ohne den Cache zu deaktivieren.

Für den Vergleich dieselbe Szene 30–60 Sekunden beobachten: Kommt der Ruckler
weiterhin ungefähr alle fünf Sekunden, ist er weg oder anders stark? Danach
dieselben Bewegungen/Kämpfe bzw. Raumwechsel wiederholen. Spielstand, Begleiter,
Re:Fined und `version.dll` unverändert lassen.

Für den Test das Spiel zweimal vollständig neu starten. Nach jedem Start muss
die Datei erneut genau eine aktuelle Statuszeile enthalten. Mehrere Raumwechsel,
Gegner-Spawns und Rikus Session testen; die Datei darf dabei nicht wachsen.
Zur Rückmeldung genügt `KH2AssetSizeCache.log` plus die Beobachtung, ob Laden und
Spielen normal funktionieren. Bei einem Absturz zusätzlich den Proton-Log schicken.
Andere DLLs wie eine Diagnose-`version.dll` können unabhängig weiter protokollieren.

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
Fehlgeschlagene Größenabfragen werden nicht gespeichert; unsichere Abfragen gehen
über den ursprünglichen Ladepfad. Änderungen externer Dateien ohne passende
Metadatenänderung werden spätestens bei Ablauf des Eintrags erneut abgefragt.

Die erste Abfrage, notwendige Aktualisierungen und tatsächliche Ladevorgänge
bleiben bestehen. Deshalb ist dies kein Versprechen, alle Standbilder zu beseitigen.

Der zusätzliche Pfad-Cache merkt sich bestätigte fehlende Dateien/Pfade für
fünf Sekunden. Neu angelegte Dateien können entsprechend verzögert erkannt
werden; Mods nur bei geschlossenem Spiel bauen oder wechseln. Die Ablaufzeiten
bleiben für den Vergleich unverändert. Weitere Details: [Pfad-Cache](NEGATIVE_PATHS.md).
