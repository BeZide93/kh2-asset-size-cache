# Loading-stall diagnostics (v1-d3)

The d1 instrumentation is retained alongside the [negative-path optimization](NEGATIVE_PATHS.md). The 256-entry, one-second positive size cache stays active
with the same validity rules. This build does not skip loads, resource
registration or remastered processing. The path cache additionally reuses confirmed missing-path probes for five seconds.

## Test and rollback

1. Close KH2. Back up the working `KH2AssetSizeCache.dll` **outside** all mod
   loading folders. Replace it with this branch's root DLL in `mod/kh2/dll`,
   **after** any OpenKH mod build. Alternatively import this branch's
   `packages/KH2-Asset-Size-Cache-v1-p2.zip` as a local OpenKH mod.
2. Keep the existing `version.dll`, Re:Fined, assets and settings unchanged.
   Do not re-enable `KH2RikuSizeFix.dll` alongside this cache.
3. While the game is closed, move the old cache, diagnostic, Panacea performance
   and hook-fix logs aside. Start one fresh session from the same save.
4. Repeat the same room transition five times in both directions. Trigger the
   same enemy spawns at least three times. Note the room, enemy and approximate
   time of the largest pause. Also check that Riku/Session, textures, sound and
   menus still work. Play for about 3–5 minutes.
5. Wait at least ten seconds after the last event, then exit normally so the
   background writer has time to flush. Send logs from this one run:

   - `KH2AssetSizeCache.log` beside the add-on;
   - **`KH2AssetLoadDiag.log`** beside the add-on;
   - `panacea_riku_perf.log` from the existing diagnostic Panacea;
   - `panacea_hookfix.log` for startup/installation verification.

The cache log must contain `INSTALLED:` **and** `DIAG INSTALLED:` and `PATHCACHE INSTALLED:`. If it reports
`DIAG DISABLED`, send that log first: the cache may still work, but no detailed
phase diagnosis is available. A stats-worker warning also means the logs will
not be flushed periodically. No Proton log or crash dump is needed for a normal
run; include the fresh Proton log if the test crashes or fails to start.

To roll back, close the game and restore the backed-up cache DLL. If installed
through OpenKH, disable the diagnostic mod and re-enable/rebuild the previous
cache mod. The stable v1 ZIP remains under `packages/`. There is no `version.dll`
replacement, registry change or save conversion to undo.

## What the log measures

Each row aggregates one file/phase within a roughly five-second drain interval.
It contains call count, total and maximum **QPC ticks**, unsuccessful-call count
and bytes returned by `fread`. Convert ticks to milliseconds with
`ticks * 1000 / frequency` using the session header. These are elapsed wall
times, including waits; they are not CPU time or measured frame durations.

| Phase | Timed operation |
| --- | --- |
| `raw_attributes` | Raw-file existence check via `GetFileAttributesW` |
| `path_lookup` | `TransformFilePath`, including its directory-priority checks |
| `package_lookup` | Original game package lookup inside the selected Panacea routine |
| `open` | `_wfopen` |
| `read` | `fread`, including actual returned byte count |
| `seek`, `tell`, `close` | `fseek`, `ftell`, `fclose` respectively |
| `remastered` | Entire `GetRemasteredFiles`, including cache checks, folder scanning, parsing and metadata updates where performed |
| `register` | Game `SetResourceItem` call |
| `size_total` | Entire original size query on a cache miss or uncached fallback |

`scope=load` covers the ordinary `LoadFile` routine; `LoadFileWithSize`,
`LoadFileWithMalloc` and separate remastered-asset reads are not instrumented by
these call sites. Keep the existing Panacea performance log for whole-load
timings and those other entry points. The retained package lookup on **cache
hits** is not included in `scope=size`. Raw decompression/decryption and other
untimed work can remain in the difference from whole-load timings.

`size_total` includes the other size phases; `remastered` is also an inclusive
measurement. Do not add nested phases or subtract independently aggregated
maxima to claim an exclusive duration for one load. File names can differ
between a game path and the resolved mod path. Escapes retain control characters
and path separators safely. Oversized names use an explicit marker.

`unsuccessful` includes expected missing-path probes, not just errors. Cumulative
`dropped`, `untracked`, `clock_errors` and `write_errors` expose incomplete
telemetry. High values must be considered before drawing conclusions. Recording
uses bounded memory and nonblocking locks. A busy/full recorder drops only a
measurement; the original operation still runs. CRT stream-to-path bookkeeping
has 32 slots, and aggregation has 2048 entries with sixteen probes per key.
The writer opens the diagnostic log once per drain; hooks do not write logs.
Instrumentation and background writes still have some overhead.

## Compatibility and implementation

The cache retains its existing automatic recognition. The extra diagnosis is
intentionally restricted to the supplied MSVC Panacea implementation: complete
caller and relevant callee hashes, section checks and the expected attribute
import must match before any diagnostic patch is attempted. Unknown builds
disable the detailed diagnostics and new path cache, retaining a supported size cache. This is not a promise to support every future
Panacea release without updating a diagnostic profile.

The add-on redirects 34 validated call-site operands and one seven-byte fallback target-selection span to nearby leaf jump relays.
The new span selects a leaf relay that restores the HD argument and tail-jumps
to the original-load wrapper. The existing Hook object, Patch/Unpatch calls and
original target remain intact. It does not detour these functions' prologues or replace their unwind metadata.
Typed wrappers preserve arguments, results and Win32 `LastError`; indirect game
and import slots are read again on each call. Modules and relays stay pinned.
Installation occurs during `OnInit`, before gameplay; failed transactions restore
original operands where writable and keep relay memory alive for safe forwarding.

The compiled PE passes cache/concurrency/discovery tests and diagnostic ABI,
timing, capacity, error-state and transaction-rollback tests on x86-64 Linux.
Local profile checks also pass against the supplied original Panacea, our stable
proxy and our performance-diagnostic proxy, with the cache detour installed.
Those external DLLs are not distributed or used in CI. Actual KH2 under Proton
Experimental ARM64 still needs the user test above.

See [d3 load details](LOAD_DETAILS.md) for the additional original-load phase,
slow-call records and thread CPU timing limitations.
