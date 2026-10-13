# Build and validate

Repository: [BeZide93/kh2-asset-size-cache](https://github.com/BeZide93/kh2-asset-size-cache).
The root DLL is ready for OpenKH installation. `packages/KH2-Asset-Size-Cache-v1-d3.zip`
contains the complete source layout and also works as a local OpenKH mod.

## Linux build

Use an x86-64 Linux environment for the compiled-code harnesses. The DLL can be
cross-compiled elsewhere, but these harnesses execute AMD64 code directly.
Requirements: Python 3.12, venv/pip, GCC, binutils (`objdump`), and the pinned Python
dependencies in `requirements.txt`.

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python build.py
gcc -O2 -pthread -I build tests/harness.c -o build/harness
build/harness KH2AssetSizeCache.dll
gcc -O2 -pthread -I build tests/discovery.c -o build/discovery
build/discovery KH2AssetSizeCache.dll
gcc -O2 -pthread -I build tests/diagnostics.c -o build/diagnostics
build/diagnostics KH2AssetSizeCache.dll
gcc -O2 -pthread -I build tests/negative_paths.c -o build/negative_paths
build/negative_paths KH2AssetSizeCache.dll
.venv/bin/python scripts/package_mod.py
```

Optional additional tests with your own original supported Panacea DLL:

```sh
build/harness KH2AssetSizeCache.dll /absolute/path/to/original/version.dll
build/diagnostics KH2AssetSizeCache.dll /absolute/path/to/original/version.dll
```

`build.py` checks the DLL and creates `BUILD-MANIFEST.json`, `PE-EXPORTS.txt` and
the test header. Packaging creates `dist/KH2-Asset-Size-Cache-v1-d3.zip` with the full
source/repository layout and OpenKH manifest, plus `dist/SHA256SUMS.txt`.
Unused CodeView/PDB metadata is stripped at link time so different checkout
paths produce the same DLL. Generated build files, PDBs and import libraries
are excluded from the ZIP.

## GitHub Actions

The workflow runs on pushes, pull requests and manual requests. It builds the
DLL, verifies it matches the committed test DLL, runs the compiled-code tests, checks repeat-build reproducibility, packages
the complete project and publishes a workflow artifact named
`KH2-Asset-Size-Cache`. No secrets, repository writes or releases are required.

After changing the code or documentation, rebuild and test, then regenerate the
ZIP and copy it from `dist` to `packages`. Keep the committed DLL synchronized
with the source. Update the root `SHA256SUMS.txt` for the repository files and
package. Build directories, PDBs and import libraries must not be committed.
