# d4: file I/O and wait/lock diagnosis

The d3 run confirms long original loads with both high and low calling-thread
CPU time. d4 adds a bounded observation of named Windows imports to correlate
these operations with the existing slow-call boundaries. This is not an
asset-data cache or a new claimed performance optimization.

## Coverage

At OnInit, after successful d3 profile installation, the DLL validates the
AMD64 PE import tables of the main game executable and Panacea/version.dll.
It recognizes named imports from KERNEL32, KERNELBASE and api-ms-win-core API
sets for these APIs:

- ReadFile; SetFilePointerEx
- GetOverlappedResult; GetOverlappedResultEx
- WaitForSingleObject / Ex; WaitForMultipleObjects / Ex
- Sleep / Ex; EnterCriticalSection
- AcquireSRWLockExclusive / Shared

Each installed slot produces an IOHOOK line in KH2AssetSizeCache.log naming
the module, API and slot RVA. IO INSTALLED means some slots were installed,
not that every listed API or all game I/O is covered. In particular, check
whether there are game-module ReadFile and wait/lock slots.

This does not hook imports of other DLLs (including a separately loaded CRT),
delay imports, dynamically resolved pointers, direct NtReadFile/NtWait calls,
memory-mapped reads, ReadFileEx/completion-port APIs, internal decompression,
graphics APIs or custom synchronization. Unknown internals are not patched at
guessed addresses. The game's executable was not supplied for offline
disassembly, so the actual game coverage must be checked in the startup log.

IAT installation preserves the previous resolved targets, including existing
third-party wrappers. There are separate wrappers/targets for each module/API.
Ambiguous duplicate targets or malformed tables disable I/O installation.
Aligned pointer exchanges use compare-and-swap; rollback never overwrites an
unexpected third-party pointer. No game function prologue is changed by d4.
If the independent I/O transaction fails, d3 diagnostics and both caches remain.

## Capture window and formats

The original_load and register wrappers maintain a global active-window count.
Outside these windows the I/O wrappers immediately forward without sampling
clocks or recording. Inside a window, calls from either covered module and
any thread can be recorded. There are no stack-local context pointers or TLS
objects retained by the recorder.

The same KH2AssetLoadDiag.log contains:

- IOSTATUS: installed/live slots, open_windows, dropped and clock_errors.
  Reduced live_slots means a slot no longer points directly to this wrapper;
  do not assume continued coverage even if another hook chains it.
  open_windows can be nonzero during a load; an exceptional unwind that skips
  normal return can also leave it nonzero. This can extend collection, but
  never retains a dangling stack pointer or changes a game result.
- IOCOUNTERS: cumulative counts, inclusive QPC total/max ticks and observed
  output bytes per module/API, including calls below the event threshold.
- IOEVENT: absolute start/end QPC, thread, module/API, handle_or_address,
  requested_or_arg, bytes, raw result, preserved last_error and timeout_ms.
  Every ReadFile and completion query is recorded; other APIs produce an
  event when they last at least 1 ms.

Only successful synchronous ReadFile output is counted as read bytes.
Asynchronous/pending ReadFile does not dereference or reuse its output buffer
or OVERLAPPED structure, and returns exactly as before. ERROR_IO_PENDING is
not a load failure. Completion-query bytes are reported separately and may
refer to writes/other operations; never sum them with ReadFile as disk bytes.
A zero bytes field need not mean a zero-byte read. LastError on successful
calls may be stale; interpret it according to the API result.

For seeks, requested_or_arg is the seek method. For multiple waits it is the
handle count; for Ex/completion calls it can be alertable/bWait. This is timing
telemetry, not a replay of complete API arguments. The handle_or_address field
is a lock/file handle or the handle-array address, not the game asset path.

## Correlation and limits

Use thread ID plus absolute QPC timestamps to match events to DETAIL boundaries.
The matching DETAIL provides the logical asset name. Events from background
threads cannot be assigned to one asset merely because they overlap in time.
A slow ReadFile measures time inside that API, not necessarily physical disk
latency. Lock-acquisition time includes uncontended overhead and contention.

Do not sum nested API durations or add them to the inclusive original-load
duration. The bundled read-only helper computes the union of recorded,
fully-contained same-thread intervals, excluding crossing events:

    python3 scripts/analyze_io.py KH2AssetLoadDiag.log

It selects the last declared log session and outputs JSON. It does not label
uncovered time as decompression or CPU work: absent hooks, short waits,
unrecorded threads and dropped events can all leave gaps.

Storage is fixed: 64 IAT slots, 26 module/API counters and 1024 events. Recording
uses a nonblocking lock; lock contention may omit both counters and an event,
queue saturation omits the event only. IOSTATUS dropped exposes either case.
Hooks perform no allocation or log-file I/O. The existing worker drains events
roughly every five seconds, outside recorder locks. QPC calls and recording
still add overhead, especially around frequent critical-section calls during
loads. These diagnostic timings are not a performance benchmark.

## Test / rollback

See DIAGNOSE_DE.md. Use the same scene/mod setup as d3, fresh logs, repeated
room transitions and enemy spawns. Expect v1-d4, DETAIL INSTALLED and IO INSTALLED.
Keep version.dll unchanged. Send all four established logs; additionally send
a fresh Proton log if the game crashes or does not start.

Restore the previous d3 DLL or retained d3 ZIP with the game closed to roll back.
