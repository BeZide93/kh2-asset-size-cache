# Findings from the supplied d1 run, 2026-10-13

All four supplied logs contain a single run. Cache and diagnostic installation
succeeded. The final cache snapshot has **3828 candidates, 3085 hits, 743 original
refreshes**, zero evictions and zero contention: about **80.6% hits**. Different
scenes and durations prevent treating this as a controlled comparison with v1.

The phase log covers approximately 145.55 seconds from its own QPC origin and
reports **272 dropped measurements**, no untracked streams, clock errors or log
write errors. Totals below describe recorded samples only; missing samples and
non-atomic drain boundaries prevent exact percentage accounting.

| Recorded phase | Calls | Total elapsed ms | Largest call ms |
| --- | ---: | ---: | ---: |
| Original size query, inclusive | 735 | 17096.2890 | 90.2528 |
| Path resolution inside size queries | 731 | 14980.5755 | 85.2764 |
| Raw-file attribute probes inside size queries | 734 | 1536.8401 | 4.9344 |
| Opening files inside size queries | 298 | 175.4302 | 2.1396 |
| Seek inside size queries | 285 | 3.4923 | 0.3053 |
| Tell inside size queries | 279 | 1.3272 | 0.2707 |
| Path resolution inside LoadFile | 221 | 2618.6762 | 45.1670 |
| Resource registration inside LoadFile | 61 | 1174.4232 | 241.8897 |
| Remastered helper inside LoadFile | 53 | 255.3995 | 33.4446 |

Of the recorded size path-resolution calls, **432 returned false**. For example,
`WM_CURSOR.a.us` has a single 85.2764 ms path lookup and a 90.2528 ms original
size query in the same bucket. `fseek`/`ftell` are not the main measured issue in
this run, so replacing them is lower priority than avoiding repeated failed
existence checks. The next patch caches only confirmed API error 2/3 responses;
the new counters will verify how often those responses occur and are reused.

There are separate residual stalls. `P_EX100.mdlx` resource registration takes
241.8897 ms and later 226.5574 ms. The outer performance log records
`M_EX710.a.us` loads of 986.2083 and 963.5102 ms. Recorded failed mod-path lookups
for that asset are consistent with Panacea entering its original-game fallback.
The detailed instrumentation does not split that game's internal loader; these
large outer times must not be attributed to the small measured mod-file reads.

The outer log's 241 LoadFile calls total 27554.6590 ms, but nested size/open calls
must not be added to that figure as independent wall time. Diagnostic and outer
log QPC origins differ by 3.7606146 seconds, so bucket numbers are not equivalent.
Independent maxima must not be subtracted to infer the remainder of one load.

Priority: cache repeated confirmed missing-path probes, retain telemetry, then
use the controlled device comparison to choose the next independent patch.
Do not skip resource registration or suppress the actual game load.
