# Testbuild v1-d1: verbleibende Ladepausen messen

Dieser Patch ergänzt die Diagnose. Der bisherige Größen-Cache bleibt aktiv;
weitere Performance-Optimierungen folgen nach Auswertung der neuen Messungen.
**Die vorhandene `version.dll` bleibt erhalten.**

1. Spiel beenden. Die bisherige `KH2AssetSizeCache.dll` außerhalb aller
   Mod-Ladeordner sichern und durch die neue DLL in `mod/kh2/dll` ersetzen.
   Erst die Mods in OpenKH bauen, dann die DLL kopieren. Alternativ die
   `KH2-Asset-Size-Cache-v1-d1.zip` aus `packages` als lokalen Mod installieren.
2. Re:Fined, andere Mods und Einstellungen unverändert lassen. Keinen
   zusätzlichen `KH2RikuSizeFix.dll` laden.
3. Bei beendetem Spiel die alten unten genannten Logs verschieben oder löschen.
4. Denselben Spielstand laden. Fünfmal zwischen denselben Räumen hin- und
   herwechseln und dieselben Gegner mindestens dreimal spawnen lassen.
   Die stärksten Pausen mit Raum/Gegner und ungefährer Zeit notieren.
   Auch Rikus Session, Menüs, Texturen und Ton kurz prüfen.
5. Insgesamt etwa 3–5 Minuten testen, nach dem letzten Ereignis mindestens
   zehn Sekunden warten und das Spiel normal beenden.

Bitte diese vier Logs **aus demselben Testlauf** schicken:

- **`KH2AssetLoadDiag.log`** neben der neuen DLL;
- `KH2AssetSizeCache.log` daneben;
- `panacea_riku_perf.log` der bisherigen Diagnose-Panacea;
- `panacea_hookfix.log` zur Prüfung der Initialisierung.

Im Cache-Log müssen `INSTALLED:` und **`DIAG INSTALLED:`** stehen. Bei
`DIAG DISABLED` zuerst das Cache-Log schicken. Der Cache kann trotzdem aktiv
sein; die zusätzliche Diagnose erkennt diesen Panacea-Build dann nicht oder
konnte nicht installiert werden. Bei einer Warnung zum Statistik-Thread fehlen
die regelmäßigen Log-Ausgaben. Einen frischen Proton-Log brauchen wir zusätzlich
nur bei einem Absturz oder Startproblem.

Die neue Logdatei trennt Pfadsuche, Paketabfrage, Öffnen, Lesen, Seek/Tell,
Schließen, Remastered-Verarbeitung und Ressourcenregistrierung. Die Werte sind
Laufzeiten pro Datei/Phase; sie ersetzen keine FPS- oder Framezeitmessung.
Die Diagnose selbst verursacht etwas Zusatzaufwand. Details und Grenzen stehen
in [DIAGNOSTICS.md](DIAGNOSTICS.md).

**Rollback:** Spiel beenden und die gesicherte Cache-DLL zurückkopieren. Bei
OpenKH-Installation den Diagnose-Mod deaktivieren und den bisherigen Cache-Mod
wieder aktivieren und bauen. Die stabile v1-ZIP bleibt in `packages` verfügbar.
Keine Änderung an `version.dll`, Registry oder Spielstand erforderlich.
