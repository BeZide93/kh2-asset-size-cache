# Testbuild v1-p2: Cache für fehlende Dateipfade

Dieser Patch speichert bestätigte fehlende Dateipfade für fünf Sekunden und
vermeidet damit wiederholte erfolglose Pfadprüfungen. Der bisherige Größen-Cache
und die Diagnose bleiben aktiv.
**Die vorhandene `version.dll` bleibt erhalten.**

1. Spiel beenden. Die bisherige `KH2AssetSizeCache.dll` außerhalb aller
   Mod-Ladeordner sichern und durch die neue DLL in `mod/kh2/dll` ersetzen.
   Erst die Mods in OpenKH bauen, dann die DLL kopieren. Alternativ die
   `KH2-Asset-Size-Cache-v1-p2.zip` aus `packages` als lokalen Mod installieren.
2. Re:Fined, andere Mods und Einstellungen unverändert lassen. Keinen
   zusätzlichen `KH2RikuSizeFix.dll` laden.
3. Bei beendetem Spiel die alten unten genannten Logs verschieben oder löschen.
4. Denselben Spielstand laden. Fünfmal zwischen denselben Räumen hin- und
   herwechseln und dieselben Gegner mindestens dreimal spawnen lassen.
   Wiederholungen einmal möglichst direkt und einmal nach mindestens zehn Sekunden
   durchführen, um Verhalten innerhalb und außerhalb der Cache-Gültigkeit zu prüfen.
   Die stärksten Pausen mit Raum/Gegner und ungefährer Zeit notieren.
   Auch Rikus Session, Menüs, Texturen und Ton kurz prüfen.
5. Insgesamt etwa 3–5 Minuten testen, nach dem letzten Ereignis mindestens
   zehn Sekunden warten und das Spiel normal beenden.

Bitte diese vier Logs **aus demselben Testlauf** schicken:

- **`KH2AssetLoadDiag.log`** neben der neuen DLL;
- `KH2AssetSizeCache.log` daneben;
- `panacea_riku_perf.log` der bisherigen Diagnose-Panacea;
- `panacea_hookfix.log` zur Prüfung der Initialisierung.

Im Cache-Log müssen `INSTALLED:` und **`DIAG INSTALLED:`** sowie **`PATHCACHE INSTALLED:`** stehen. Bei
`DIAG DISABLED` zuerst das Cache-Log schicken. Der Cache kann trotzdem aktiv
sein; die zusätzliche Diagnose erkennt diesen Panacea-Build dann nicht oder
konnte nicht installiert werden. Bei einer Warnung zum Statistik-Thread fehlen
die regelmäßigen Log-Ausgaben. Einen frischen Proton-Log brauchen wir zusätzlich
nur bei einem Absturz oder Startproblem.

`PATHCACHE`-Zeilen im Cache-Log zeigen `hits`, echte `queries` und `expired`.
Die erste Prüfung eines Pfades und Prüfungen nach Ablauf der fünf Sekunden
bleiben bestehen. Neue Dateien können für fünf Sekunden, bereits gespeicherte
Größenantworten zusammen mit dem bisherigen Cache bis ungefähr sechs Sekunden
verzögert sichtbar werden. Mods deshalb bei beendetem Spiel wechseln/bauen.

Die neue Logdatei trennt Pfadsuche, Paketabfrage, Öffnen, Lesen, Seek/Tell,
Schließen, Remastered-Verarbeitung und Ressourcenregistrierung. Die Werte sind
Laufzeiten pro Datei/Phase; sie ersetzen keine FPS- oder Framezeitmessung.
Die Diagnose selbst verursacht etwas Zusatzaufwand. Details und Grenzen stehen
in [DIAGNOSTICS.md](DIAGNOSTICS.md).

**Rollback:** Spiel beenden und die gesicherte Cache-DLL zurückkopieren. Bei
OpenKH-Installation den Diagnose-Mod deaktivieren und den bisherigen Cache-Mod
wieder aktivieren und bauen. Die stabile v1-ZIP bleibt in `packages` verfügbar.
Keine Änderung an `version.dll`, Registry oder Spielstand erforderlich.
