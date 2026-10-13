#!/usr/bin/env python3
"""Produce a complete local OpenKH mod; never upload or mutate a repository."""
import hashlib
import pathlib
import zipfile
import pefile

ROOT = pathlib.Path(__file__).resolve().parents[1]
DLL = ROOT / "KH2AssetSizeCache.dll"
manifest = ROOT / "mod.yml"
if not DLL.is_file():
    raise SystemExit("Missing KH2AssetSizeCache.dll: run python3 build.py first.")
p = pefile.PE(str(DLL))
assert p.FILE_HEADER.Machine == 0x8664 and p.OPTIONAL_HEADER.Magic == 0x20b
assert p.FILE_HEADER.Characteristics & 0x2000
assert [(e.name, e.ordinal) for e in p.DIRECTORY_ENTRY_EXPORT.symbols] == [(b"OnInit", 1)]
assert [d.dll.lower() for d in p.DIRECTORY_ENTRY_IMPORT] == [b"kernel32.dll"]
assert p.OPTIONAL_HEADER.CheckSum == p.generate_checksum()
# Keep this deliberately single-asset package synchronized with its manifest.
expected = ("game: kh2", "- name: dll/KH2AssetSizeCache.dll", "  method: copy",
            "  platform: pc", "  - name: KH2AssetSizeCache.dll")
lines = manifest.read_text(encoding="utf-8").splitlines()
assert all(line in lines for line in expected)
assert sum(line.startswith("- name:") for line in lines) == 1
files = ["mod.yml", DLL.name, "README.md", "LICENSE", "LICENSE-HDE.txt",
         "LICENSE-OpenKH.txt", "NOTICE.txt", "requirements.txt", "build.py",
         ".gitignore", ".github/workflows/build.yml"]
for folder in ("docs", "src", "tests", "scripts"):
    files.extend(str(p.relative_to(ROOT)) for p in sorted((ROOT / folder).rglob("*"))
                 if p.is_file() and p.suffix in (".md", ".c", ".h", ".py"))
assert len(files) == len(set(files))
out = ROOT / "dist"
out.mkdir(exist_ok=True)
archive = out / "KH2-Asset-Size-Cache-v1-d1.zip"
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    checksums = []
    for name in files:
        data = (ROOT / name).read_bytes()
        checksums.append(hashlib.sha256(data).hexdigest() + "  " + name + "\n")
        entry = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
        entry.compress_type = zipfile.ZIP_DEFLATED
        entry.external_attr = 0o100644 << 16
        z.writestr(entry, data)
    entry = zipfile.ZipInfo("SHA256SUMS.txt", date_time=(2026, 1, 1, 0, 0, 0))
    entry.compress_type = zipfile.ZIP_DEFLATED
    entry.external_attr = 0o100644 << 16
    z.writestr(entry, "".join(checksums))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert z.read(DLL.name) == DLL.read_bytes()
    assert set(z.namelist()) == set(files) | {"SHA256SUMS.txt"}
    for line in z.read("SHA256SUMS.txt").decode().splitlines():
        digest, name = line.split("  ", 1)
        assert hashlib.sha256(z.read(name)).hexdigest() == digest
(out / "SHA256SUMS.txt").write_text("".join(
    hashlib.sha256(f.read_bytes()).hexdigest() + "  " + f.name + "\n"
    for f in (DLL, archive)), encoding="utf-8")
print(f"PASS: complete OpenKH PC mod ZIP, {archive.stat().st_size} bytes: {archive}")
