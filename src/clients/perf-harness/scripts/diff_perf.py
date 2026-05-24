#!/usr/bin/env python3
# Shortcircuit XT - a Surge Synth Team product
# MIT license, see LICENSE.
#
# Diff two scxt-perf JSON reports. Surfaces timing deltas, fingerprint match,
# and warns if config / source file hashes differ ("you compared apples to
# oranges").

import json
import math
import sys


def fmt_pct(old, new):
    if old == 0:
        return f"  (was 0, now {new:.3f})"
    pct = (new - old) / old * 100.0
    sign = "+" if pct >= 0 else ""
    return f"  ({sign}{pct:.1f}%)"


def warn_diff(label, a, b):
    if a != b:
        print(f"  ⚠ {label}: {a} != {b}  — comparison may not be meaningful")


def main():
    if len(sys.argv) != 3:
        print("Usage: diff_perf.py <old.json> <new.json>", file=sys.stderr)
        sys.exit(2)

    old = json.load(open(sys.argv[1]))
    new = json.load(open(sys.argv[2]))

    print(f"diff: {sys.argv[1]}  →  {sys.argv[2]}")
    print(f"  config: {old.get('config_name')}  →  {new.get('config_name')}")
    warn_diff("config_hash", old.get("config_hash"), new.get("config_hash"))
    warn_diff("file_hash  ", old.get("load", {}).get("file_hash"),
              new.get("load", {}).get("file_hash"))
    warn_diff("scxt_build_type", old.get("scxt_build_type"), new.get("scxt_build_type"))
    print()

    a = old.get("aggregate", {})
    b = new.get("aggregate", {})
    print("Timing:")
    for k in ("wall_ms_median", "wall_ms_stddev", "block_ns_p99_median"):
        ov = a.get(k, 0.0)
        nv = b.get(k, 0.0)
        print(f"  {k:24s} {ov:12.3f}  →  {nv:12.3f}{fmt_pct(ov, nv)}")

    # Significance: 2σ on the wall-ms metric
    median_o = a.get("wall_ms_median", 0.0)
    median_n = b.get("wall_ms_median", 0.0)
    sd = max(a.get("wall_ms_stddev", 0.0), b.get("wall_ms_stddev", 0.0), 1e-6)
    delta = abs(median_n - median_o)
    flag = "  ⚠ outside 2σ" if delta > 2 * sd else "  (within 2σ)"
    print(f"  wall_ms delta:           {delta:12.3f}{flag}")
    print()

    fpa = old.get("fingerprint", {})
    fpb = new.get("fingerprint", {})
    exact_match = fpa.get("exact_fnv1a") == fpb.get("exact_fnv1a")
    print(f"Fingerprint exact: {'MATCH' if exact_match else 'DIFFER'}")
    print(f"  old: {fpa.get('exact_fnv1a')}")
    print(f"  new: {fpb.get('exact_fnv1a')}")

    if not exact_match:
        ta = fpa.get("tolerant", {})
        tb = fpb.get("tolerant", {})
        print()
        print("Tolerant fingerprint (audio actually changed?):")
        for k in ("peak", "rms"):
            ov = ta.get(k, 0.0)
            nv = tb.get(k, 0.0)
            print(f"  {k:8s} {ov:12.6f}  →  {nv:12.6f}{fmt_pct(ov, nv)}")
        zo = ta.get("zero_crossings", 0)
        zn = tb.get("zero_crossings", 0)
        print(f"  zero_crossings  {zo}  →  {zn}")

        ea = ta.get("envelope_per_100ms", [])
        eb = tb.get("envelope_per_100ms", [])
        if ea and eb and len(ea) == len(eb):
            max_dev = max(abs(x - y) for x, y in zip(ea, eb))
            print(f"  envelope max dev: {max_dev:.6f}")
        elif ea or eb:
            print(f"  envelope length differs: {len(ea)} vs {len(eb)}")


if __name__ == "__main__":
    main()
