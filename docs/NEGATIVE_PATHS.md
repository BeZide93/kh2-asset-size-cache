# Missing-path cache without diagnostic logging

This build transfers only the performance behavior of PR #2 onto the startup-only
main build. The positive size cache still uses 256 entries and a fixed one-second
TTL. The separate negative path cache uses 1,024 entries and a fixed five-second
TTL from the actual query's start; hits do not extend the deadline.

Only `GetFileAttributesW` results with `ERROR_FILE_NOT_FOUND` (2) or
`ERROR_PATH_NOT_FOUND` (3) are cached. Existing files, directories and all other
errors are always queried. Keys are exact UTF-16 drive-absolute paths up to 259
code units, plus the current attribute callback. Relative, UNC/device and long
paths bypass caching. Return values and Windows LastError are preserved.

The fixed cache uses a nonblocking lock. Contention runs the original query;
generation and in-flight checks prevent stale hit/publication across these
fallback observations. There is no per-request allocation, logging or worker.

## Installation and compatibility

Three complete original Panacea function bodies (LoadFile, GetFileSize and
TransformFilePath), their instruction operands and the GetFileAttributesW import
are checked before installing the size detour. The optional path-cache patch then
changes only five four-byte call displacements: three attribute probes in
TransformFilePath and one raw-file probe in each of LoadFile and GetFileSize.
All use the same cache and call the current original import on misses.

The five patches are installed as one startup transaction with rollback. The
relay is retained on a transaction failure so a surviving call can still forward
without caching. The independently installed size cache remains active if the
path profile is unsupported or optional patch setup fails. This is not a promise
of compatibility with future recompiled Panacea versions.

No load-phase wrappers, CPU measurements, I/O/wait hooks, diagnostic buffers,
periodic logs or logging thread from PRs #1–#4 are included. The single startup
line reports `path_cache=ACTIVE; entries=1024; path_TTL=5000 ms` on success or
`path_cache=DISABLED` with a reason. It replaces the previous run's log.

## Limits and comparison test

A newly created file can remain invisible to an old negative entry for up to
five seconds, with up to another second for a dependent size answer. A resource
already loaded by the game is not automatically reloaded when the TTL expires.
Close the game before rebuilding or changing mods.

Repeated queries can still incur a synchronous cost after an entry expires.
This build intentionally keeps both TTLs unchanged, to test whether the reported
five-second hitch persists without the old PR #2 diagnostic/logging overhead.
It does not guarantee stutter-free asset loading.

Replace only KH2AssetSizeCache.dll after OpenKH builds the mods. Keep the same
save, scene, companions, Re:Fined, version.dll and other settings. First confirm
`path_cache=ACTIVE`. Observe the same scene for 30–60 seconds, then repeat the
same movement/combat or room changes. Report whether the regular hitch is gone,
still about every five seconds, or has a different interval/intensity. Restart
once and confirm the log remains a single fresh line. Return KH2AssetSizeCache.log
and the observation; include a Proton log only for a crash.

For rollback, close the game and restore the previous main/startup-only DLL or
rebuild that mod version. The previous ZIP remains in packages.
