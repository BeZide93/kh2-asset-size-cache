# KH2 Asset Size Cache

Reduces repeated asset-size lookup overhead in **Kingdom Hearts II Final Mix**.
Windows x64 Panacea add-on, including under Proton on ARM64.

**Diagnostic test build `v1-d3`:** retains both size and missing-path caches.
Adds a separate measurement of the original game's `LoadFile` fallback, plus
per-call wall time and current-thread CPU time for slow fallback and resource
registration calls. This is a diagnostic step, not another claimed FPS fix.
Keep your existing `version.dll`.
See [German test instructions](docs/DIAGNOSE_DE.md) and
[measurement details and limitations](docs/LOAD_DETAILS.md).

## How it works

Caches successful size queries for all supported file paths, without a list of
specific actors or extensions. A fixed cache holds up to **256 entries**, each
valid for at most **one second**. The game's package lookup still runs on every
candidate; package and caller context must match before a result is reused.
The size cache does not store missing files, errors or unstable results.
The separate missing-path cache stores only confirmed absence as described above.

This includes the repeated Riku/Session queries covered by KH2 Riku Performance
Fix and the additional resource checks made by Re:Fined. It replaces that add-on.
It does not cache asset contents or change AI, animations or save data. Initial
loads and graphics-related stalls may remain.

## Install

1. Close the game. Disable/remove **KH2 Riku Performance Fix** in OpenKH and
   rebuild your mods. Remove any remaining `KH2RikuSizeFix.dll` from the active
   `mod/kh2/dll` folder; keep a backup outside the loader folders.
2. In OpenKH Mod Manager, select **KH2 / PC** and install
   **`BeZide93/kh2-asset-size-cache`** from GitHub. Enable it and build/apply
   your mods. Alternatively import the [complete ZIP](packages/KH2-Asset-Size-Cache-v1-d3.zip)
   as a local mod. For this unmerged test build, use the ZIP or PR branch DLL;
   a normal GitHub install from main does not select this build. No compilation is needed.
3. Alternatively copy `KH2AssetSizeCache.dll` directly into `mod/kh2/dll` after
   building your mods. Keep your existing Panacea `version.dll`.
4. Restart the game. `KH2AssetSizeCache.log` beside the DLL should show
   `slots=256` and `INSTALLED:`. `DISABLED:` means no cache hook was installed.

Run only one size-cache add-on. Do not rename this DLL to `version.dll`.
See [German installation and rollback](docs/INSTALL_DE.md).

## Compatibility and validation

Compatible Panacea functions are detected automatically, without fixed build
addresses. Unknown or ambiguous implementations safely disable the cache. The path optimization and diagnostics use a separate, stricter profile;
`DIAG DISABLED` leaves an otherwise supported size cache active.
The cache uses nonempty paths up to 259 bytes and requires valid package metadata;
other requests retain the original behavior. External file changes without a
metadata change may remain invisible for up to one second.

Compiled DLL tests pass, including concurrency, cache expiry and automatic
Panacea discovery. User testing of the base cache v1 on Proton ARM64 reports substantially improved
performance; the supplied log shows about 88% cache hits. Actual asset-loading
stalls remain, and this does not establish compatibility with every mod or scene.
See [technical details](docs/COMPATIBILITY.md) and [build/CI](docs/BUILD.md).

Newly added mod files may remain hidden from a cached missing-path probe for up
to five seconds, with up to one additional second for an already cached size
result. Restart the game after rebuilding or switching mods.

## Uninstall

Close the game, disable this mod in OpenKH and rebuild. Remove any manually
installed `KH2AssetSizeCache.dll` still in the output folder. To return to the
Riku-only fix, restore/re-enable it after removing this add-on, then restart.

## License

Apache-2.0; HDE64 uses its included BSD-style license. See `NOTICE.txt`.
