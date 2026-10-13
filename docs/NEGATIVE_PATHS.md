# Missing-path cache: v1-p2

The d1 logs identify unsuccessful `TransformFilePath` calls as the main measured
cost of remaining original size queries. This patch targets those probes before
considering changes to actual asset loads. It builds on diagnostic PR #1.

The existing 256-entry positive size cache retains its one-second TTL. A separate
1024-entry, four-way cache now remembers **only** `GetFileAttributesW` failures
with `ERROR_FILE_NOT_FOUND` (2) or `ERROR_PATH_NOT_FOUND` (3), for five seconds
from the original query's start. Hits do not extend that deadline. Successes,
directories and all other errors always use the original API. Return values and
Win32 last-error values are retained.

Keys are complete, exact UTF-16 drive-absolute paths up to 259 code units and the
current attribute-function pointer. Relative, drive-relative, UNC/device and
long paths bypass caching. There is no filename/extension/actor whitelist, case
folding, directory-wide absence inference or permanent negative result.
Dev/mod/extract priority and the game's package-selection side effects remain.
Raw mod probes inside `LoadFile`/`GetFileSize` and the three attribute calls
inside `TransformFilePath` use the cache. The latter also benefits other Panacea
callers of that helper. Other processes and global Wine imports are untouched.

One nonblocking lock protects the fixed cache. Contention calls the original API;
an in-flight counter and generation checks prevent stale reuse/publication
overlapping those fallback observations. No allocation, polling or file logging
occurs per request. `PATHCACHE` cumulative statistics appear every five seconds
in `KH2AssetSizeCache.log`, including hits, actual queries, expiration, eviction,
bypasses, contention and observed error categories.

New mod files can be hidden from a cached absence probe for at most five seconds;
the existing one-second size cache can add another second to a derived size
answer. **Close the game before switching/rebuilding mods.** First-time probes
and probes after expiry still incur the original cost. This patch cannot promise
to remove resource-registration, base-game loading or GPU stalls.

The new path cache deliberately uses the same strict Panacea profile and
transaction as the diagnosis. A mismatch disables these additions and leaves
the separately recognized size cache active. It does not add a universal
future-build compatibility guarantee. The diagnostic recorder now has 2048 rows
and sixteen probes instead of 512/eight because the supplied run lost 272
measurements; its memory remains bounded.

## Test

Replace only `mod/kh2/dll/KH2AssetSizeCache.dll` while the game is closed. Keep
the current `version.dll`, Re:Fined and other mods unchanged. Preserve the previous
DLL outside loader folders for rollback. Start with all four logs cleared/moved.

Use the same save and rooms as d1. Repeat transitions five times and enemy spawns
at least three times, including both immediate repetition and repetition after
ten seconds. Check normal resource loading, textures, sound, menus and Session.
Wait ten seconds after the last event, then exit normally. Send:

- `KH2AssetLoadDiag.log`
- `KH2AssetSizeCache.log`
- `panacea_riku_perf.log`
- `panacea_hookfix.log`

Confirm `PATHCACHE INSTALLED`, `DIAG INSTALLED` and the size-cache `INSTALLED`
messages. Report the room/enemy and whether pauses improved, stayed the same or
became worse. Add a fresh Proton log for crashes only. Roll back by closing KH2
and restoring the previous cache DLL, or restoring the previous OpenKH mod.

Local compiled-DLL tests cover negative/positive/error cases, key separation,
fixed expiry and creation after expiry, callback replacement, bypasses, and real
concurrent fallback invalidation. Existing cache/ABI/discovery/rollback tests are
retained. Device performance and compatibility remain to be confirmed.

API contract: [GetFileAttributesW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesw)
and [Windows error codes](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-).
