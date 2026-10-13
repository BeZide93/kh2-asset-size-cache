# d3: original-load and resource-registration diagnosis

The supplied p2 run confirms that both caches work. The four recorded
`eh04.map` loads still take 800–816 ms while their replacement-path searches
take only 0.002–0.006 ms. Resource registration for `P_EX100.mdlx` takes 264 ms.
This build measures the previously unseparated original-game fallback.

## Measurements

- `PHASE scope=load phase=original_load`: the original game's LoadFile call
  reached after Panacea finds no raw/ordinary replacement. Every completed
  call is aggregated, including calls shorter than the slow-call threshold.
- `DETAIL phase=original_load` and `DETAIL phase=register`: individual calls
  lasting at least 20 ms, with absolute QPC start/end, wall ticks, thread ID,
  signed return value, file name and user/kernel CPU deltas.
- `register` continues to measure the complete game's SetResourceItem call.
  It does not yet separate its internal functions.

Wall milliseconds = `wall_ticks * 1000 / frequency`.
CPU milliseconds = `(kernel_100ns + user_100ns) / 10000`, only when
`cpu_valid=1`. Absolute timestamps allow comparison across logs even when
their origins and bucket numbers differ.

The CPU samples bracket the QPC samples, so include a little instrumentation
overhead. CPU time is for the calling thread only, not all game threads.
A low CPU/wall ratio can indicate waiting, scheduling or work elsewhere; it
does **not** identify disk I/O, GPU work or a hardware bottleneck. Timer
resolution and Wine/Proton accounting matter; zero CPU deltas alone are not
proof of inactivity. Unsupported/failed or backward CPU readings produce
`cpu_valid=0` and increase `cpu_errors`, without changing the game result.

API reference:
https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes

## Scope and overhead

Only Panacea's ordinary LoadFile fallback and its two direct SetResourceItem
call sites receive the additional timing. LoadFileWithSize/WithMalloc, direct
game calls elsewhere, GPU operations and individual decompression/read
functions are not newly instrumented. Original-load time can itself include
resource work. Do not add inclusive parent/child durations.

There are two GetThreadTimes calls per instrumented call. QPC/CPU sampling
adds overhead; this build is for diagnosis and does not promise shorter pauses.
Slow events enter a fixed 256-entry queue through a nonblocking lock.
`detail_dropped` counts events omitted due to lock contention or a full queue;
`dropped` separately counts omitted PHASE measurements. Neither skips game work.
A background worker writes both formats to KH2AssetLoadDiag.log, roughly every
five seconds. Hooks allocate no memory and perform no file logging.

## Installation and safety

Replace only KH2AssetSizeCache.dll after the OpenKH mod build, with the game
closed. Keep version.dll, Re:Fined, assets, settings and both cache TTLs unchanged.
Check `v1-d3`, `PATHCACHE INSTALLED`, `DIAG INSTALLED` and `DETAIL INSTALLED`.
Unknown Panacea diagnostic profiles disable these diagnostics and the path
cache; the separately recognized positive size cache may remain available.

The exact profiled caller replaces seven bytes that select its original target
and HD argument with a nearby relay address. The leaf relay restores the HD
argument and tail-jumps to a normally unwindable Windows-x64 C wrapper.
The wrapper reads the original target from Panacea's existing Hook object
each time. It works with the original target or our stable trampoline and
does not replace the Hook object or add a game-entry detour. Concurrent
third-party replacement of Hook objects is not a supported scenario.

Installation validates the full caller/callee profiles and commits all 35
patch spans in one transaction before gameplay. Failure rolls back the spans;
if protection restoration fails, relay memory remains alive for safe forwarding.
Local compiled-PE tests cover the relay, both HD values, target replacement,
arguments/results/LastError, mixed-length rollback, timing errors and queue
overflow. The profile/patch tests pass against the supplied original, stable
and diagnostic Panacea binaries. They are not a real game/Proton ARM64 test.

Rollback: restore the previous p2 DLL or import the retained p2 ZIP, rebuild
mods and restart. No version.dll, registry or save changes are required.
