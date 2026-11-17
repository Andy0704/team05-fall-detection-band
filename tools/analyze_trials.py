#!/usr/bin/env python3
"""Per-trial fall-rule diagnostics from a raw log + trial sheet.

Usage: python3 tools/analyze_trials.py <log> <trials.csv> [labels.csv]

Each trial is analysed over its whole loop window (counter_start ..
counter_end, or counter_ready .. counter_lie_end for operator-paced sheets),
and the landing is taken from the SVM peak, not from cues or key presses, because the operators did not
always act exactly on the cue. Within that window it finds the largest SVM
peak and reports what each stage of the firmware rule would see:

  - longest free-fall run (SVM < FREEFALL_THRESHOLD_G) in the 1 s before
    the peak, vs FREEFALL_MIN_MS
  - peak SVM vs IMPACT_THRESHOLD_G
  - tilt from vertical (a_v = -ax, firmware/src/fall_detector.cpp) at the
    peak sample and its median while lying (last 3 s before the get-up cue,
    or 1-3 s after the peak for old sheets), vs
    TILT_THRESHOLD_DEG
  - which axis carries gravity while lying (same window as tilt_lie)
  - states the firmware logged in the window

Thresholds are copied from firmware/include/config.h; keep them in sync.
Optional labels.csv (trial,activity) overrides the sheet's activity column.
"""
import csv
import math
import statistics
import sys

FREEFALL_THRESHOLD_G = 0.4
FREEFALL_MIN_MS = 50
IMPACT_THRESHOLD_G = 2.5
TILT_THRESHOLD_DEG = 60.0
SAMPLE_MS = 10


def load_log(path):
    data = {}
    for line in open(path, errors="replace"):
        p = line.strip().split(",")
        if len(p) == 6 and p[0].isdigit() and p[2]:
            try:
                data[int(p[0])] = (float(p[2]), float(p[3]), float(p[4]), p[5])
            except ValueError:
                pass
    return data


def trial_window(r):
    """(lo, hi, lie_end) counters for a trial row, for either sheet format.

    Old fixed-timer sheets: counter_start..counter_end. Operator-paced sheets
    (record_trials.py since 4d31239): counter_ready..counter_lie_end, the
    lying phase ends at counter_lie_end.
    """
    if r.get("counter_start"):
        return int(r["counter_start"]), int(r["counter_end"]), None
    if r.get("counter_ready") and r.get("counter_lie_end"):
        return int(r["counter_ready"]), int(r["counter_lie_end"]), int(r["counter_lie_end"])
    return None


def tilt(ax, ay, az):
    svm = math.sqrt(ax * ax + ay * ay + az * az)
    return math.degrees(math.acos(max(-1.0, min(1.0, -ax / svm)))) if svm else float("nan")


def main():
    log, sheet = sys.argv[1], sys.argv[2]
    labels = {}
    if len(sys.argv) > 3:
        labels = {int(r["trial"]): r["activity"] for r in csv.DictReader(open(sys.argv[3]))}
    data = load_log(log)
    print(f"log: {log}\nsheet: {sheet}")
    print("trial activity        peak_g  peak_c  ff_ms  min_g  tilt@pk  tilt_lie  lie_axis   states")
    for r in csv.DictReader(open(sheet)):
        t = int(r["trial"])
        act = labels.get(t, r["activity"])
        w = trial_window(r)
        if w is None:
            print(f"{t:5d} {act:<15} skipped ({r.get('notes', '')})")
            continue
        lo, hi, lie_end = w
        win = [(c, data[c]) for c in range(lo, hi) if c in data]
        svm = {c: math.sqrt(v[0] ** 2 + v[1] ** 2 + v[2] ** 2) for c, v in win}
        pk = max(svm, key=svm.get)
        # longest free-fall run in 1 s before the peak
        run = best = 0
        for c in range(pk - 100, pk + 1):
            if c in svm and svm[c] < FREEFALL_THRESHOLD_G:
                run += 1
                best = max(best, run)
            else:
                run = 0
        pre = [svm[c] for c in range(pk - 100, pk + 1) if c in svm]
        # lying posture: last 3 s before the "get up" cue if known, else
        # 1-3 s after the peak (old fixed-timer sheets)
        lie_rng = range(lie_end - 300, lie_end) if lie_end else range(pk + 100, pk + 300)
        lie = [data[c] for c in lie_rng if c in data]
        if lie:
            med = [statistics.median(v[i] for v in lie) for i in range(3)]
            tl = tilt(*med)
            i = max(range(3), key=lambda k: abs(med[k]))
            axis = f"{'+' if med[i] > 0 else '-'}{'xyz'[i]}({med[i]:+.2f})"
        else:
            tl, axis = float("nan"), "n/a"
        states = sorted({v[3] for _, v in win})
        print(f"{t:5d} {act:<15} {svm[pk]:6.2f} {pk:7d} {best * SAMPLE_MS:6d} {min(pre):6.2f}"
              f" {tilt(*data[pk][:3]):8.1f} {tl:9.1f}  {axis:<10} {'|'.join(states)}")
    print(f"\nrule: free-fall < {FREEFALL_THRESHOLD_G} g for >= {FREEFALL_MIN_MS} ms -> "
          f"impact > {IMPACT_THRESHOLD_G} g -> tilt > {TILT_THRESHOLD_DEG} deg -> still 2 s")


if __name__ == "__main__":
    main()
