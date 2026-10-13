# Test von v1-d4

Dies ist ein Diagnose-Patch. Er behält die beiden funktionierenden Caches bei
und ergänzt die bisherigen Zeitmessungen um Datei-Lesezugriffe und
Windows-Warte-/Sperraufrufe. Die Entpackroutine wird noch nicht direkt gemessen. Eine weitere FPS-Verbesserung ist noch nicht
das Ziel dieses Builds.

1. KH2 vollständig schließen. Die bisherige KH2AssetSizeCache.dll außerhalb
   aller Mod-Ladeordner sichern.
2. Die neue DLL nach dem letzten OpenKH-Mod-Build nach
   `mod/kh2/dll/KH2AssetSizeCache.dll` kopieren. Alternativ das d4-ZIP als lokale
   OpenKH-Mod importieren. Ein normaler GitHub-Import von main enthält diesen
   noch nicht gemergten Teststand nicht.
3. version.dll, Re:Fined, andere Mods und Einstellungen unverändert lassen.
   Nur eine KH2AssetSizeCache.dll laden; KH2RikuSizeFix.dll nicht zusätzlich laden.
4. Die vier bisherigen Logs bei geschlossenem Spiel verschieben oder löschen.
   Nach dem Start muss KH2AssetSizeCache.log `v1-d4`, `PATHCACHE INSTALLED`,
   `DIAG INSTALLED`, `DETAIL INSTALLED` und `IO INSTALLED` enthalten.
   Die `IOHOOK`-Zeilen zeigen die tatsächlich erfassten APIs. Falls `IO DISABLED`
   erscheint oder keine Game-I/O-Hooks vorhanden sind, diese Logs ebenfalls schicken.
5. Den gleichen Spielstand wie beim letzten d3-Test laden. Die auffälligen Raumwechsel
   fünfmal wiederholen, möglichst genau dieselbe Strecke wie zuvor (zuletzt hb02/hb03).
   Die gleichen Gegner mindestens dreimal spawnen lassen. Raum/Gegner und
   ungefährer Zeitpunkt besonders langer Pausen helfen bei der Zuordnung.
6. Texturen, Sound, Menüs und Riku/Session kurz prüfen. Etwa 3–5 Minuten spielen;
   nach dem letzten Ereignis zehn Sekunden warten und normal beenden.

Bitte diese vier frischen Logs aus demselben Lauf schicken:

- KH2AssetLoadDiag.log
- KH2AssetSizeCache.log
- panacea_riku_perf.log
- panacea_hookfix.log

Im ersten Log erwarten wir `KH2AssetLoadDiag d4`,
`PHASE scope=load phase=original_load` und bei langen Aufrufen
`DETAIL phase=original_load` bzw. `DETAIL phase=register`.
`cpu_valid=0` bitte ebenfalls melden; die Wall-Time bleibt dann auswertbar.
Zusätzlich erwarten wir `IOSTATUS`, `IOCOUNTERS` und `IOEVENT` im Diagnose-Log.
`live_slots` sollte `installed_slots` entsprechen; `dropped` bei IOSTATUS zeigt
fehlende API-Messdaten an. Dieser Diagnose-Build kann zusätzlichen Overhead
verursachen; eine längere Pause allein belegt daher keine Verschlechterung des
ursprünglichen Ladewegs.
Ohne Crash wird kein neuer Proton-Log benötigt. Bei einem Crash oder fehlendem
Start zusätzlich den frischen Proton-Log schicken.

Optional für später: derselbe Test ohne Asset-Mods, soweit der Spielstand und
die Mod-Abhängigkeiten das zulassen. Logs getrennt aufbewahren. Für den ersten
d4-Test bitte die bisherige Konfiguration beibehalten.

Rollback: bei geschlossenem Spiel die gesicherte d3-DLL wiederherstellen,
gegebenenfalls die Mod-Ausgabe neu bauen. version.dll bleibt unverändert.
