#!/usr/bin/env python3
"""Record a fall-trial session, paced by the operator and synced to the log.

Each loop is anchored to what actually happened, not to a timer:

  1. READY    operator presses SPACE when the faller stands in position
  2. STAND    5 s countdown, spoken "stand still"           (auto)
  3. FALL     spoken "fall"; operator presses SPACE the moment the body
              lands on the mat                              (key = landed)
  4. LIE      12 s countdown from the landing key, spoken "lie still";
              the faller lies FULLY down (back, front or side) and does
              not move                                       (auto)
  5. GET UP   spoken "cancel, get up"; operator presses the cancel button
              if the buzzer is on, faller gets up -> next loop's READY

Every phase boundary and key press is stamped with the board's live sample
counter, so the trial sheet indexes straight into the raw log. After the
session, each trial is checked against the data itself: the largest SVM
peak between the fall cue and landing + 2 s must lie within SYNC_TOL_S of
the landing key, otherwise the trial is flagged sync_ok=no (or no_impact
if no peak exceeds SYNC_MIN_PEAK_G).

Keys:  SPACE = confirm / landed    s = skip this trial (noted in sheet)
       q = stop the session (sheet is still written)

The raw serial stream is saved byte-for-byte (never edited). The trial
sheet is written next to it. Run in your own Terminal window:

    ~/.platformio/penv/bin/python tools/record_trials.py \
        --port /dev/cu.usbmodem5CBB0142471 \
        --name 2026-11-18_falls_session3 \
        --activity fall-backward,fall-left,fall-forward,...

--activity: one label per loop, in the order decided BEFORE recording
(do not relabel after seeing results). The number of labels sets the
number of loops. Labels starting with "fall" are expected to alarm.

--auto: presses keys automatically on fixed delays (dry-run/testing only).
"""
import argparse
import math
import os
import select
import subprocess
import sys
import termios
import threading
import time
import tty

import serial

BAUD = 115200
STAND_S = 5         # baseline standing still before the fall cue
LIE_S = 12          # >= POST_IMPACT_SETTLE_MS (1 s) + INACTIVITY_MS (2 s), with margin
SYNC_TOL_S = 1.0    # max distance between landing key and SVM peak
SYNC_MIN_PEAK_G = 1.5  # design choice: below this there was no real landing (flag only)
PEAK_SEARCH_AFTER_LAND_S = 2.0
SAMPLE_HZ = 100     # nominal, only used to convert seconds to counter steps

CLEAR = "\033[2J\033[H"
BOLD = "\033[1m"
END = "\033[0m"


def say(text):
    """Spoken cue, non-blocking. Silently skipped if `say` is unavailable."""
    try:
        subprocess.Popen(["say", text], stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    except OSError:
        pass


class SerialRecorder(threading.Thread):
    """Writes raw bytes to file; keeps the latest counter/state and samples."""

    def __init__(self, ser, path):
        super().__init__(daemon=True)
        self.ser = ser
        self.f = open(path, "xb")  # "x": never overwrite an existing log
        self.buf = b""
        self.counter = None
        self.state = None
        self.header_seen = False
        self.samples = {}  # counter -> (ax, ay, az, state), current boot only
        self.stop_flag = False

    def run(self):
        while not self.stop_flag:
            data = self.ser.read(4096)
            if not data:
                continue
            self.f.write(data)
            self.buf += data
            *lines, self.buf = self.buf.split(b"\n")
            for raw in lines:
                line = raw.decode("ascii", "replace").strip()
                if line.startswith("counter,"):
                    self.header_seen = True
                    self.samples = {}
                    continue
                p = line.split(",")
                if self.header_seen and len(p) == 6 and p[0].isdigit():
                    c = int(p[0])
                    if p[2]:
                        try:
                            self.samples[c] = (float(p[2]), float(p[3]), float(p[4]), p[5])
                        except ValueError:
                            pass
                    self.counter, self.state = c, p[5]
        self.f.flush()
        self.f.close()


class Keys:
    """Non-blocking single-key reads from the terminal (or scripted keys)."""

    def __init__(self, auto):
        self.auto = auto
        self.next_auto = None
        if not auto:
            self.fd = sys.stdin.fileno()
            self.old = termios.tcgetattr(self.fd)
            tty.setcbreak(self.fd)

    def restore(self):
        if not self.auto:
            termios.tcsetattr(self.fd, termios.TCSADRAIN, self.old)

    def get(self, auto_delay_s):
        """Return a pressed key or None. In auto mode, SPACE after a delay."""
        if self.auto:
            now = time.monotonic()
            if self.next_auto is None:
                self.next_auto = now + auto_delay_s
            if now >= self.next_auto:
                self.next_auto = None
                return " "
            return None
        r, _, _ = select.select([sys.stdin], [], [], 0)
        return sys.stdin.read(1) if r else None


def screen(rec, header, title, lines, footer=""):
    sys.stdout.write(CLEAR + header + "\n\n")
    sys.stdout.write(f"{BOLD}   >>> {title} <<<{END}\n\n")
    for l in lines:
        sys.stdout.write(f"   {l}\n")
    if footer:
        sys.stdout.write(f"\n   {BOLD}{footer}{END}\n")
    sys.stdout.write(f"\n   board: counter={rec.counter} state={rec.state}\n")
    sys.stdout.write("   keys: SPACE confirm/landed   s skip trial   q stop session\n")
    sys.stdout.flush()


def wait_key(rec, keys, header, title, lines, auto_delay_s):
    """Show a screen until SPACE/s/q. Returns (key, counter at key press)."""
    while True:
        screen(rec, header, title, lines)
        k = keys.get(auto_delay_s)
        if k in (" ", "s", "q"):
            return k, rec.counter
        time.sleep(0.05)


def countdown(rec, keys, header, title, lines, seconds):
    """Timed screen. Returns 's'/'q' if pressed during it, else None."""
    end = time.monotonic() + seconds
    while True:
        left = end - time.monotonic()
        if left <= 0:
            return None
        screen(rec, header, title, lines, f"{left:4.1f} s")
        k = None if keys.auto else keys.get(0)
        if k in ("s", "q"):
            return k
        time.sleep(0.05)


def sync_check(samples, c_fall, c_land):
    """Largest SVM peak between fall cue and landing + 2 s."""
    if c_fall is None or c_land is None:
        return None, None, "n/a"
    hi = c_land + int(PEAK_SEARCH_AFTER_LAND_S * SAMPLE_HZ)
    svm = {c: math.sqrt(v[0] ** 2 + v[1] ** 2 + v[2] ** 2)
           for c in range(c_fall, hi + 1) if (v := samples.get(c))}
    if not svm:
        return None, None, "no_data"
    pk = max(svm, key=svm.get)
    if svm[pk] < SYNC_MIN_PEAK_G:
        return pk, svm[pk], "no_impact"
    ok = abs(pk - c_land) <= SYNC_TOL_S * SAMPLE_HZ
    return pk, svm[pk], "yes" if ok else "no"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--name", required=True)
    ap.add_argument("--activity", required=True,
                    help="comma-separated labels, one per loop, fixed before recording")
    ap.add_argument("--outdir", default="data/raw")
    ap.add_argument("--auto", action="store_true", help="scripted key presses (testing)")
    ap.add_argument("--no-voice", action="store_true")
    args = ap.parse_args()

    acts = [a.strip() for a in args.activity.split(",") if a.strip()]
    log_path = os.path.join(args.outdir, args.name + ".log")
    sheet_path = os.path.join(args.outdir, args.name + "_trials.csv")
    for p in (log_path, sheet_path):
        if os.path.exists(p):
            sys.exit(f"refusing to overwrite {p}")
    speak = (lambda t: None) if (args.no_voice or args.auto) else say

    ser = serial.Serial(args.port, BAUD, timeout=0.1)
    ser.reset_input_buffer()
    ser.dtr = False
    ser.rts = True   # reset the board so the log header is captured
    time.sleep(0.1)
    ser.rts = False
    rec = SerialRecorder(ser, log_path)
    rec.start()

    t_boot = time.monotonic()
    while not (rec.header_seen and rec.counter is not None):
        if time.monotonic() - t_boot > 10:
            rec.stop_flag = True
            sys.exit("no log header / samples within 10 s - check port and board")
        time.sleep(0.05)

    keys = Keys(args.auto)
    trials = []
    t0 = time.monotonic()
    try:
        for i, act in enumerate(acts):
            hdr = f"Loop {i + 1}/{len(acts)}   activity: {act}   (elapsed {time.monotonic() - t0:.0f} s)"
            row = dict(trial=i + 1, activity=act,
                       expected="fall" if act.startswith("fall") else "no_alarm",
                       notes="")
            speak("get ready")
            k, row["counter_ready"] = wait_key(
                rec, keys, hdr, "GET READY",
                [f"Next: {act}",
                 "Faller: stand on the mat, still. Spotter in position.",
                 "Board state should be MONITORING.",
                 "Operator: press SPACE when ready."], 1.0)
            if k == "q":
                break
            if k == "s":
                row["notes"] = "skipped at ready"
                trials.append(row)
                continue

            speak("stand still")
            k = countdown(rec, keys, hdr, "STAND STILL", ["Stand upright, do not move."], STAND_S)
            if k:
                row["notes"] = "skipped during stand"
                trials.append(row)
                if k == "q":
                    break
                continue

            speak("fall")
            row["counter_fall_cue"] = rec.counter
            k, row["counter_landed"] = wait_key(
                rec, keys, hdr, "FALL NOW",
                [f"Fall onto the mat: {act}",
                 "Spotter: support head and neck.",
                 "Operator: press SPACE the moment the body LANDS."], 1.5)
            if k in ("s", "q"):
                row["counter_landed"] = None
                row["notes"] = "skipped before landing"
                trials.append(row)
                if k == "q":
                    break
                continue

            speak("lie still")
            k = countdown(rec, keys, hdr, "LIE STILL",
                          ["Lie FULLY down (back, front or side).",
                           "Do not move until the next cue."], LIE_S)
            row["counter_lie_end"] = rec.counter
            if k:
                row["notes"] = "skipped during lie"
            speak("cancel, get up")
            row["states_seen"] = "|".join(sorted({
                v[3] for c, v in rec.samples.items()
                if row["counter_fall_cue"] <= c <= row["counter_lie_end"]}))
            trials.append(row)
            if k == "q":
                break
            # GET UP is folded into the next loop's READY screen
    finally:
        keys.restore()
        elapsed = time.monotonic() - t0
        time.sleep(0.5)
        rec.stop_flag = True
        rec.join(timeout=2)
        ser.close()

    cols = ["trial", "activity", "expected", "buzzer_YN", "states_seen",
            "counter_ready", "counter_fall_cue", "counter_landed", "counter_lie_end",
            "peak_counter", "peak_svm_g", "sync_ok", "notes"]
    with open(sheet_path, "x") as f:
        f.write(",".join(cols) + "\n")
        for r in trials:
            pk, pv, ok = sync_check(rec.samples, r.get("counter_fall_cue"), r.get("counter_landed"))
            r.update(peak_counter=pk, peak_svm_g=None if pv is None else f"{pv:.2f}", sync_ok=ok)
            f.write(",".join("" if r.get(c) is None else str(r.get(c)) for c in cols) + "\n")

    sys.stdout.write(CLEAR)
    print(f"Done. {len(trials)} trials, {elapsed:.0f} s.")
    print(f"raw log:     {log_path}")
    print(f"trial sheet: {sheet_path}  (fill in buzzer_YN by hand)\n")
    for r in trials:
        det = "FALL_DETECTED" in (r.get("states_seen") or "")
        print(f"  trial {r['trial']:2d} {r['activity']:<15} detected={'YES' if det else 'no '}"
              f"  sync_ok={r['sync_ok']:<7} peak={r.get('peak_svm_g') or '-'} g  {r['notes']}")
    bad = [r["trial"] for r in trials if r["sync_ok"] == "no"]
    if bad:
        print(f"\n  WARNING: trials {bad} - SVM peak not within {SYNC_TOL_S} s of the landing key.")
    weak = [r["trial"] for r in trials if r["sync_ok"] == "no_impact"]
    if weak:
        print(f"  NOTE: trials {weak} - no SVM peak above {SYNC_MIN_PEAK_G} g near the landing key.")


if __name__ == "__main__":
    main()
