#!/usr/bin/env python3
"""Replay a raw log through a Python copy of the firmware fall rule.

Usage:
  python3 tools/simulate_rule.py <log> [--sheet trials.csv] [--labels labels.csv]
                                 [--rule v2|v1] [--freefall-min-ms 50] [...]

--rule v2 (default) mirrors firmware/src/fall_detector.cpp as of
2026-11-17 (free-fall -> impact -> SETTLE -> lying still with tilt).
--rule v1 is the previous rule (tilt at impact, still check seeded with the
impact SVM), kept only to check this copy against what the board logged
in runs recorded on that firmware: with --rule v1 on such a log, the
"agreement" line should be close to 100%.

Defaults are copied from firmware/include/config.h; keep them in sync.
Every parameter can be overridden on the command line for tuning, which
must only use the tuning set (CLAUDE.md data rules), never held-out trials.

With --sheet, prints per trial whether the simulated rule reached
FALL_DETECTED inside that trial's loop window (counter_start..counter_end).
After a simulated detection the detector is reset on the next sample, like
a cancel press, so later trials are unaffected.
"""
import argparse
import csv
import math

DEFAULTS = dict(
    freefall_threshold_g=0.4,
    freefall_min_ms=50,
    impact_threshold_g=2.5,
    impact_window_ms=500,
    tilt_threshold_deg=60.0,
    post_impact_settle_ms=1000,
    inactivity_ms=2000,
    inactivity_svm_dev_g=0.15,
    inactivity_timeout_ms=6000,
    standing_svm_g=1.0,
)


def tilt_deg(ax, ay, az, svm):
    ratio = max(-1.0, min(1.0, -ax / svm))  # a_v = -ax (fall_detector.cpp)
    return math.degrees(math.acos(ratio))


class Detector:
    def __init__(self, p, rule):
        self.p, self.rule = p, rule
        self.state = "MONITORING"
        self.t0 = self.t_imp = 0
        self.lo = self.hi = 0.0

    def update(self, valid, ax, ay, az, us):
        p, S = self.p, self.state
        if not valid:
            if S in ("FREE_FALL", "IMPACT_WAIT", "SETTLE", "INACTIVITY_CHECK"):
                self.state = "MONITORING"
            return self.state
        svm = math.sqrt(ax * ax + ay * ay + az * az)
        el = ((us - self.t0) & 0xFFFFFFFF) // 1000
        if S == "MONITORING":
            if svm < p["freefall_threshold_g"]:
                self.state, self.t0 = "FREE_FALL", us
        elif S == "FREE_FALL":
            if svm < p["freefall_threshold_g"]:
                if el >= p["freefall_min_ms"]:
                    self.state, self.t0 = "IMPACT_WAIT", us
            else:
                self.state = "MONITORING"
        elif S == "IMPACT_WAIT":
            if el > p["impact_window_ms"]:
                self.state = "MONITORING"
            elif svm > p["impact_threshold_g"]:
                if self.rule == "v1":
                    if tilt_deg(ax, ay, az, svm) > p["tilt_threshold_deg"]:
                        self.state, self.t0 = "INACTIVITY_CHECK", us
                        self.lo = self.hi = svm
                    else:
                        self.state = "MONITORING"
                else:
                    self.state, self.t0, self.t_imp = "SETTLE", us, us
        elif S == "SETTLE":
            if el >= p["post_impact_settle_ms"]:
                self.state, self.t0 = "INACTIVITY_CHECK", us
                self.lo = self.hi = svm
        elif S == "INACTIVITY_CHECK":
            self.lo, self.hi = min(self.lo, svm), max(self.hi, svm)
            moved = (self.hi - p["standing_svm_g"] > p["inactivity_svm_dev_g"]) or \
                    (p["standing_svm_g"] - self.lo > p["inactivity_svm_dev_g"])
            if self.rule == "v1":
                if moved:
                    self.state = "MONITORING"
                elif el >= p["inactivity_ms"]:
                    self.state = "FALL_DETECTED"
            else:
                if moved:
                    self.t0 = us
                    self.lo = self.hi = svm
                elif el >= p["inactivity_ms"]:
                    self.state = ("FALL_DETECTED"
                                  if tilt_deg(ax, ay, az, svm) > p["tilt_threshold_deg"]
                                  else "MONITORING")
                    return self.state
                if ((us - self.t_imp) & 0xFFFFFFFF) // 1000 > p["inactivity_timeout_ms"]:
                    self.state = "MONITORING"
        return self.state


def load(path):
    rows = []
    for line in open(path, errors="replace"):
        f = line.strip().split(",")
        if len(f) != 6 or not f[0].isdigit() or not f[1].isdigit():
            continue
        try:
            if f[2]:
                rows.append((int(f[0]), int(f[1]), True, float(f[2]), float(f[3]), float(f[4]), f[5]))
            else:
                rows.append((int(f[0]), int(f[1]), False, 0.0, 0.0, 0.0, f[5]))
        except ValueError:
            pass
    # keep only the last boot (counter restarts at reset)
    start = max((i for i in range(1, len(rows)) if rows[i][0] < rows[i - 1][0]), default=0)
    return rows[start:]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--sheet")
    ap.add_argument("--labels")
    ap.add_argument("--rule", choices=["v1", "v2"], default="v2")
    for k, v in DEFAULTS.items():
        ap.add_argument("--" + k.replace("_", "-"), type=type(v), default=v)
    a = ap.parse_args()
    p = {k: getattr(a, k) for k in DEFAULTS}

    rows = load(a.log)
    det = Detector(p, a.rule)
    sim = {}
    agree = 0
    reached = set()
    for c, us, valid, ax, ay, az, logged in rows:
        s = det.update(valid, ax, ay, az, us)
        sim[c] = s
        reached.add(s)
        agree += (s == logged)
        if s == "FALL_DETECTED":
            det.state = "MONITORING"  # emulate cancel / reset()
    print(f"log: {a.log}  rule: {a.rule}  samples: {len(rows)}")
    print("params: " + " ".join(f"{k}={v}" for k, v in p.items()))
    print(f"agreement with logged state column: {agree}/{len(rows)} "
          f"({100.0 * agree / len(rows):.2f}%)  [meaningful only for logs from the same rule]")
    print(f"states reached in simulation: {'|'.join(sorted(reached))}")

    if a.sheet:
        labels = {}
        if a.labels:
            labels = {int(r["trial"]): r["activity"] for r in csv.DictReader(open(a.labels))}
        print("\ntrial activity        detected  furthest_state")
        order = ["MONITORING", "FREE_FALL", "IMPACT_WAIT", "SETTLE", "INACTIVITY_CHECK", "FALL_DETECTED"]
        for r in csv.DictReader(open(a.sheet)):
            t = int(r["trial"])
            seen = {sim[c] for c in range(int(r["counter_start"]), int(r["counter_end"])) if c in sim}
            far = max(seen, key=order.index) if seen else "n/a"
            print(f"{t:5d} {labels.get(t, r['activity']):<15} {'YES' if 'FALL_DETECTED' in seen else 'no':<9} {far}")


if __name__ == "__main__":
    main()
