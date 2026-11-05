#!/usr/bin/env python3
"""Measure sample rate and dropped-tick rate from a raw serial log.

Usage: python3 analyze_log.py <path/to/raw_log.txt>

Expects the CLAUDE.md log format: header lines starting with '#', then
data lines 'counter,micros,ax,ay,az,state'. Never edits the raw log; only
reads it.
"""
import re
import sys

DATA_LINE_RE = re.compile(r"^(\d+),(\d+),")


def analyze(path):
    rows = []
    with open(path, "r", errors="replace") as f:
        for line in f:
            m = DATA_LINE_RE.match(line)
            if m:
                rows.append((int(m.group(1)), int(m.group(2))))

    if len(rows) < 2:
        print("Not enough data rows to measure a rate.")
        return

    first_counter, first_us = rows[0]
    last_counter, last_us = rows[-1]
    expected_ticks = last_counter - first_counter
    elapsed_s = (last_us - first_us) / 1e6
    measured_hz = expected_ticks / elapsed_s if elapsed_s > 0 else float("nan")

    counters = [r[0] for r in rows]
    gaps = [
        (counters[i - 1], counters[i])
        for i in range(1, len(counters))
        if counters[i] - counters[i - 1] != 1
    ]
    missing_ticks = sum(b - a - 1 for a, b in gaps)
    drop_rate = missing_ticks / expected_ticks if expected_ticks > 0 else 0.0

    print(f"source file: {path}")
    print(f"rows parsed: {len(rows)}")
    print(f"counter range: {first_counter} -> {last_counter} ({expected_ticks} intervals)")
    print(f"elapsed: {elapsed_s:.6f} s")
    print(f"measured rate: {measured_hz:.4f} Hz")
    print(f"gaps: {len(gaps)} (missing ticks: {missing_ticks})")
    print(f"drop rate: {drop_rate:.6%}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)
    analyze(sys.argv[1])
