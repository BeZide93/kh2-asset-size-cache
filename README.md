# KH2 Asset Size Cache

Reduces repeated asset-size lookup overhead in **Kingdom Hearts II Final Mix**.
Windows x64 Panacea add-on, including under Proton on ARM64.

**This branch adds diagnostic build `v1-d1`.** The cache behavior is unchanged.
It adds `KH2AssetLoadDiag.log` to locate remaining loading stalls; it is not yet
a further performance fix. Keep your existing `version.dll`.
See the [test steps and required logs](docs/DIAGNOSTICS.md) or
[German test instructions](docs/DIAGNOSE_DE.md).

## How it works

Caches successful size queries for all supported file paths, without a list of
specific actors or extensions. A fixed cache holds up to **256 entries**, each
valid for at most **one second**. The game's package lookup still runs on every
candidate; package and caller context must match before a result is reused.
Missing files, errors and unstable results are not cached.

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
   your mods. Alternatively import the [complete ZIP](packages/KH2-Asset-Size-Cache-v1-d1.zip)
   as a local mod. The ready-to-use DLL is included; no compilation is needed.
3. Alternatively copy `KH2AssetSizeCache.dll` directly into `mod/kh2/dll` after
   building your mods. Keep your existing Panacea `version.dll`.
4. Restart the game. `KH2AssetSizeCache.log` beside the DLL should show
   `slots=256` and `INSTALLED:`. `DISABLED:` means no cache hook was installed.

Run only one size-cache add-on. Do not rename this DLL to `version.dll`.
See [German installation and rollback](docs/INSTALL_DE.md).

## Compatibility and validation

Compatible Panacea functions are detected automatically, without fixed build
addresses. Unknown or ambiguous implementations safely disable the cache. The optional
diagnostics use a separate, stricter profile; `DIAG DISABLED` does not disable
an otherwise supported cache.
The cache uses nonempty paths up to 259 bytes and requires valid package metadata;
other requests retain the original behavior. External file changes without a
metadata change may remain invisible for up to one second.

Compiled DLL tests pass, including concurrency, cache expiry and automatic
Panacea discovery. User testing of the base cache v1 on Proton ARM64 reports substantially improved
performance; the supplied log shows about 88% cache hits. Actual asset-loading
stalls remain, and this does not establish compatibility with every mod or scene.
See [technical details](docs/COMPATIBILITY.md) and [build/CI](docs/BUILD.md).

## Uninstall

Close the game, disable this mod in OpenKH and rebuild. Remove any manually
installed `KH2AssetSizeCache.dll` still in the output folder. To return to the
Riku-only fix, restore/re-enable it after removing this add-on, then restart.

## License

Apache-2.0; HDE64 uses its included BSD-style license. See `NOTICE.txt`.
