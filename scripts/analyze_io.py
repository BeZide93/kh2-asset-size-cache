#!/usr/bin/env python3
"""Correlate d4 events with slow d3/d4 boundaries; no unmeasured-time attribution."""
import argparse
import json
import re
from pathlib import Path

def fields(line):
    return dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))

def union_ticks(intervals):
    end = None
    total = 0
    for a, b in sorted(intervals):
        if end is None or a > end:
            total += b - a
            end = b
        elif b > end:
            total += b - end
            end = b
    return total

def analyze(text):
    # Logs append across starts: analyze only the last declared session.
    lines = text.splitlines()
    starts = [i for i, line in enumerate(lines) if line.startswith("KH2AssetLoadDiag ")]
    if not starts:
        raise ValueError("Missing diagnostic session header")
    lines = lines[starts[-1]:]
    freq = int(fields(lines[0])["frequency"])
    if freq <= 0:
        raise ValueError("Invalid QPC frequency")
    details = [fields(x) for x in lines if x.startswith("DETAIL ")]
    events = [fields(x) for x in lines if x.startswith("IOEVENT ")]
    statuses = [fields(x) for x in lines if x.startswith("IOSTATUS ")]
    result = []
    for d in details:
        start, end = int(d["start_qpc"]), int(d["end_qpc"])
        inside, crossing = [], 0
        for e in events:
            a, b = int(e["start_qpc"]), int(e["end_qpc"])
            if e["thread"] != d["thread"] or b <= start or a >= end:
                continue
            if a >= start and b <= end:
                inside.append(e)
            else:
                crossing += 1
        intervals = [(int(e["start_qpc"]), int(e["end_qpc"])) for e in inside]
        apis = {}
        for e in inside:
            key = e["module"] + ":" + e["api"]
            a = apis.setdefault(key, {"events": 0, "inclusive_ms": 0.0})
            a["events"] += 1
            a["inclusive_ms"] += (int(e["end_qpc"]) - int(e["start_qpc"])) * 1000 / freq
        result.append({
            "file": json.loads(d["file"]), "phase": d["phase"], "thread": int(d["thread"]),
            "start_qpc": start, "wall_ms": (end-start)*1000/freq,
            "cpu_ms": (int(d["kernel_100ns"])+int(d["user_100ns"]))/10000 if d["cpu_valid"] == "1" else None,
            "observed_api_union_ms": union_ticks(intervals)*1000/freq,
            "crossing_events_excluded": crossing, "apis": apis,
        })
    return {
        "session": lines[0],
        "coverage": statuses[-1] if statuses else None,
        "warning": "Only recorded same-thread events fully inside each boundary are attributed. API totals may overlap; use the union. Missing coverage, sub-threshold events and drops leave gaps. The remainder is NOT measured decompression or CPU work. Background-thread events are not attributed.",
        "calls": sorted(result, key=lambda x: x["wall_ms"], reverse=True),
    }

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    print(json.dumps(analyze(args.log.read_text(encoding="utf-8", errors="replace")), indent=2, ensure_ascii=False))
