# Test von v1-d3

Dies ist ein Diagnose-Patch. Er behält die beiden funktionierenden Caches bei
und misst zusätzlich den originalen Ladeweg sowie die Thread-CPU-Zeit langer
Lade- und Registrierungsaufrufe. Eine weitere FPS-Verbesserung ist noch nicht
das Ziel dieses Builds.

1. KH2 vollständig schließen. Die bisherige KH2AssetSizeCache.dll außerhalb
   aller Mod-Ladeordner sichern.
2. Die neue DLL nach dem letzten OpenKH-Mod-Build nach
   `mod/kh2/dll/KH2AssetSizeCache.dll` kopieren. Alternativ das d3-ZIP als lokale
   OpenKH-Mod importieren. Ein normaler GitHub-Import von main enthält diesen
   noch nicht gemergten Teststand nicht.
3. version.dll, Re:Fined, andere Mods und Einstellungen unverändert lassen.
   Nur eine KH2AssetSizeCache.dll laden; KH2RikuSizeFix.dll nicht zusätzlich laden.
4. Die vier bisherigen Logs bei geschlossenem Spiel verschieben oder löschen.
   Nach dem Start muss KH2AssetSizeCache.log `v1-d3`, `PATHCACHE INSTALLED`,
   `DIAG INSTALLED` und `DETAIL INSTALLED` enthalten.
5. Den gleichen Spielstand wie beim p2-Test laden. Die auffälligen Raumwechsel
   fünfmal wiederholen, möglichst dieselbe Strecke mit eh03/eh04/eh06 wie zuvor.
   Die gleichen Gegner mindestens dreimal spawnen lassen. Raum/Gegner und
   ungefährer Zeitpunkt besonders langer Pausen helfen bei der Zuordnung.
6. Texturen, Sound, Menüs und Riku/Session kurz prüfen. Etwa 3–5 Minuten spielen;
   nach dem letzten Ereignis zehn Sekunden warten und normal beenden.

Bitte diese vier frischen Logs aus demselben Lauf schicken:

- KH2AssetLoadDiag.log
- KH2AssetSizeCache.log
- panacea_riku_perf.log
- panacea_hookfix.log

Im ersten Log erwarten wir `KH2AssetLoadDiag d3`,
`PHASE scope=load phase=original_load` und bei langen Aufrufen
`DETAIL phase=original_load` bzw. `DETAIL phase=register`.
`cpu_valid=0` bitte ebenfalls melden; die Wall-Time bleibt dann auswertbar.
Ohne Crash wird kein neuer Proton-Log benötigt. Bei einem Crash oder fehlendem
Start zusätzlich den frischen Proton-Log schicken.

Optional für später: derselbe Test ohne Asset-Mods, soweit der Spielstand und
die Mod-Abhängigkeiten das zulassen. Logs getrennt aufbewahren. Für den ersten
d3-Test bitte die bisherige Konfiguration beibehalten.

Rollback: bei geschlossenem Spiel die gesicherte p2-DLL wiederherstellen,
gegebenenfalls die Mod-Ausgabe neu bauen. version.dll bleibt unverändert.
