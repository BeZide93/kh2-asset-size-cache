# Compatibility and design

## Cache scope

The add-on intercepts the recognized Panacea `GetFileSize` implementation.
There is no filename, extension, language or actor whitelist. Nonempty paths
up to 259 bytes are eligible. Positive sizes are stored only if the game's
package lookup returns stable metadata. Unsupported paths, missing package
metadata, zero/negative results and changed context retain the original routine.

A fixed 64-set, four-way cache provides 256 entries. Each set checks exact path
bytes after a 32-bit hash comparison. Case differences, slash differences and
absolute roots remain separate keys. Hash collisions can cause earlier eviction
but cannot return another path's result. Invalid/expired slots are preferred;
otherwise the set evicts entries in round-robin order. No per-query allocation.

Every cache candidate still executes `GetFileInfo`, retaining package-selection
and current-file side effects. Reuse requires equal package pointer, vtable,
header pointer, handle/count, offset, 16-byte current-file record, owner, calling
thread and lookup callback. Cached keys own their path and metadata copies.
A foreign lookup pointer disables reuse. Each entry has a fixed 1000 ms lifetime
from the start of its miss; hits never extend it, and backward time forces refresh.
External mod-file changes that do not change package metadata can remain invisible
until this deadline. Mod installation and file editing should happen with the
game closed; this is not a file-watcher or a hot-reload system.

The cache uses a nonblocking lock. Contention and unsupported requests call the
original implementation. Epochs change before and after unlocked fallback calls;
an active counter suppresses reuse/publication while they run. Results spanning
an epoch change are not reused/published. The code preserves Windows LastError
and does not perform log I/O on the request path. Initialization writes one
status line, replacing the previous log with `CREATE_ALWAYS`. There is no
periodic logging or background worker. A second `OnInit` in the same process
is ignored. File-open/write errors do not disable the cache; they can leave
an old, empty or incomplete log. If the DLL is never loaded or `OnInit` is never
called, it cannot report that failure or reset an old log.

## Automatic Panacea discovery

Startup searches the PE exception/function table for profile `msvc-size-1`,
decodes the complete routine with HDE64 and normalizes relocated references.
Function addresses, build timestamps and export RVAs are not pinned. The known
596-byte function shape, instructions, constants, package layout, imports,
section permissions, lookup pointer, required unwind data and a unique match
remain mandatory. Unknown, ambiguous or incompatible layouts disable the hook.

Only KH2's recognized Windows x64 executable is accepted. The add-on is x64 even
on an ARM64 host. It retains one `OnInit` export and imports only KERNEL32; it does
not replace Panacea's proxy DLL or its exports. Loaded modules
are pinned because the detour must remain valid for the process lifetime.

This is not universal compatibility with every future compiler/Panacea rewrite.
Layout-preserving rebuilds can be recognized; structural changes may need a new
reviewed profile. Discovery occurs once before gameplay, not per request.

## Replacing the Riku-only cache

Riku/Session files and Re:Fined's model, motion, language and limit queries are
included automatically. Remove `KH2RikuSizeFix.dll` before installation. The two
add-ons target the same routine and must not be combined. Already-modified code
is rejected by recognition; it is not chained or overwritten blindly.

## Validation and limits

The harness executes the actual Windows AMD64 DLL with the Microsoft x64 ABI
and controlled WinAPI substitutes on Linux x86-64. Tests cover:

- 1,000 repeated queries with package lookup retained and no hot-path logging;
- 900 alternating queries using seven original size lookups;
- arbitrary names, languages, extensions and exact path separation;
- all 256 slots, deliberate hash-set collisions and bounded eviction;
- TTL, backward clock, changed metadata/thread and missing/unstable results;
- real overlapping unlocked fallback, suppressing reuse and publication;
- 40,000 concurrent calls with exact accounting and unchanged results/LastError;
- synthetic relocated Panacea layouts and refusal of incompatible/ambiguous code;
- optionally, trampoline execution and OnInit with an original Panacea fixture.

Local PE checks verify architecture, exports/imports, checksum, unwind metadata
and separate writable/executable sections. The optional original-Panacea fixture
was tested locally but is not distributed. Build instructions and CI commands are
in `BUILD.md`. No in-game KH2, Wine, Proton or ARM64/FEX run was possible in the build
environment. A user test on Proton ARM64 reports substantially improved
performance; its log records 3,375 hits from 3,832 candidates with no capacity
evictions or contention fallbacks. This is not exhaustive gameplay validation.
No GPU timing, asset-content cache, asynchronous asset loading or shader cache
is provided. A size-cache hit does not avoid the actual asset load.

## Optional missing-path cache

The separate negative attribute cache and its stricter five-call-site profile
are described in [NEGATIVE_PATHS.md](NEGATIVE_PATHS.md). Its five-second validity
is independent of the positive size cache's one-second TTL. Only confirmed
missing paths are cached; this does not change the positive cache's rejection of
failed sizes. The startup line reports whether both caches or only the size
cache are installed. No diagnostic hooks or periodic logging are enabled.

## Supported proxy names

At OnInit, the add-on checks already loaded `version.dll`, `dbghelp.dll` and
`dinput8.dll` modules using GetModuleHandleW. Its documented case-insensitive
matching covers `DBGHELP.dll` and other capitalization. Each unique module
handle is validated by the same existing Panacea size-function/code/layout
profile before any hook is installed. A system DLL under one name does not
prevent a compatible Panacea under another name from being selected.

Multiple distinct compatible candidates or ambiguous code matches disable
initialization without patching any candidate. Duplicate handles are checked
once. The selected module is pinned using its address as before; the stricter
optional path-cache check uses that same module. The startup line includes the
normalized lookup name (`panacea=dbghelp.dll`, for example), not necessarily the
on-disk capitalization. No extra diagnostic logging, loader override, LoadLibrary
call or process-wide module enumeration is added. Unsupported filenames and
future incompatible builds are not covered.

API contract: [GetModuleHandleW](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlew).
Tests use a case-insensitive loader substitute and synthetic module layouts;
they do not constitute an actual Windows/Proton loader integration test.
