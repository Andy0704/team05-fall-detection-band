#!/usr/bin/env python3
"""Record a guided fall-trial session: stand > fall > lie, repeated.

Resets the board (so the log header with the firmware commit is captured),
saves the raw serial stream byte-for-byte to data/raw/, and prints timed
instructions on screen. Also writes a trial sheet next to the log with the
sample counter at each phase, taken from the live stream.

Run it in your own terminal (the countdown is interactive):

    ~/.platformio/penv/bin/python tools/record_trials.py \
        --port /dev/cu.usbmodem5CBB0142471 \
        --name 2026-11-17_fall-forward_session1 \
        --activity fall-forward

--activity takes one label (used for every loop) or a comma-separated list
with one label per loop, written down BEFORE recording (do not relabel
after seeing results).

Default timing: 10 s get-ready, then 10 loops of
stand 5 s > fall 3 s > lie still 8 s > cancel + get up 7 s = 240 s total.
Lie still must exceed INACTIVITY_S (2 s, firmware/include/config.h).
The cancel step is needed because the detector stays in FALL_DETECTED
until the button is pressed (or ALARM_TIMEOUT_S = 30 s passes), and
would not detect the next fall until then.
"""
import argparse
import os
import sys
import threading
import time

import serial

BAUD = 115200
READY_S = 10       # get into position before the first loop
STAND_S = 5        # baseline, standing still
FALL_S = 3         # window in which to perform the fall
LIE_S = 8          # lie still; > INACTIVITY_S (2 s) with margin
RESET_S = 7        # press cancel, get up, back into position
LOOPS = 10

CLEAR = "\033[2J\033[H"
BOLD = "\033[1m"
RESET_FMT = "\033[0m"


class SerialRecorder(threading.Thread):
    """Writes raw bytes to file and tracks the latest counter/state."""

    def __init__(self, ser, path):
        super().__init__(daemon=True)
        self.ser = ser
        self.f = open(path, "xb")  # "x": never overwrite an existing log
        self.buf = b""
        self.counter = None
        self.state = None
        self.header_seen = False
        self.states_since_mark = set()
        self.stop_flag = False
        self.lock = threading.Lock()

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
                    continue
                parts = line.split(",")
                if len(parts) == 6 and parts[0].isdigit():
                    with self.lock:
                        self.counter = int(parts[0])
                        self.state = parts[5]
                        self.states_since_mark.add(parts[5])
        self.f.flush()
        self.f.close()

    def mark(self):
        """Return (counter, states seen since last mark) and start a new mark."""
        with self.lock:
            states = self.states_since_mark
            self.states_since_mark = set()
            return self.counter, states


def countdown(rec, title, lines, seconds, loop_label):
    end = time.monotonic() + seconds
    while True:
        left = end - time.monotonic()
        if left <= 0:
            break
        sys.stdout.write(CLEAR)
        sys.stdout.write(f"{loop_label}\n\n")
        sys.stdout.write(f"{BOLD}   >>> {title} <<<{RESET_FMT}\n\n")
        for l in lines:
            sys.stdout.write(f"   {l}\n")
        sys.stdout.write(f"\n   {BOLD}{left:4.1f} s{RESET_FMT}\n\n")
        sys.stdout.write(f"   board: counter={rec.counter} state={rec.state}\n")
        sys.stdout.flush()
        time.sleep(0.1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--name", required=True,
                    help="log name without extension, e.g. 2026-11-17_fall-forward_session1")
    ap.add_argument("--activity", required=True,
                    help="one label, or comma-separated list with one per loop")
    ap.add_argument("--outdir", default="data/raw")
    ap.add_argument("--loops", type=int, default=LOOPS)
    args = ap.parse_args()

    acts = [a.strip() for a in args.activity.split(",") if a.strip()]
    if len(acts) == 1:
        acts = acts * args.loops
    if len(acts) != args.loops:
        sys.exit(f"--activity has {len(acts)} labels, need 1 or {args.loops}")

    log_path = os.path.join(args.outdir, args.name + ".log")
    sheet_path = os.path.join(args.outdir, args.name + "_trials.csv")
    for p in (log_path, sheet_path):
        if os.path.exists(p):
            sys.exit(f"refusing to overwrite {p}")

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

    t0 = time.monotonic()
    rows = []
    try:
        countdown(rec, "GET READY",
                  ["Faller: stand on the mat, band at the waist.",
                   "Spotter: in position.",
                   f"First loop: {acts[0]}"],
                  READY_S, "Session start")
        rec.mark()
        for i in range(args.loops):
            label = f"Loop {i + 1}/{args.loops}   activity: {acts[i]}"
            c_stand, _ = rec.mark()
            countdown(rec, "STAND STILL", ["Stand upright, do not move."],
                      STAND_S, label)
            c_fall, _ = rec.mark()
            countdown(rec, "FALL NOW", [f"Fall onto the mat: {acts[i]}",
                                        "Spotter: support head and neck."],
                      FALL_S, label)
            c_lie, s1 = rec.mark()
            countdown(rec, "LIE STILL", ["Stay down, do not move."],
                      LIE_S, label)
            c_reset, s2 = rec.mark()
            nxt = acts[i + 1] if i + 1 < args.loops else "(last loop)"
            countdown(rec, "CANCEL + GET UP",
                      ["Operator: press CANCEL if the buzzer is on.",
                       "Faller: get up, back to standing position.",
                       f"Next: {nxt}"],
                      RESET_S, label)
            c_end, s3 = rec.mark()
            states = s1 | s2 | s3
            expected = "fall" if acts[i].startswith("fall") else "no_alarm"
            rows.append((i + 1, acts[i], expected, "",
                         "|".join(sorted(states)), c_stand, c_fall, c_lie,
                         c_reset, c_end, ""))
    finally:
        elapsed = time.monotonic() - t0
        time.sleep(0.3)
        rec.stop_flag = True
        rec.join(timeout=2)
        ser.close()

    with open(sheet_path, "x") as f:
        f.write("trial,activity,expected,buzzer_YN,states_seen,counter_start,"
                "counter_fall_cue,counter_lie_cue,counter_cancel_cue,counter_end,notes\n")
        for r in rows:
            f.write(",".join("" if v is None else str(v) for v in r) + "\n")

    sys.stdout.write(CLEAR)
    print(f"Done. {len(rows)} loops, {elapsed:.1f} s guided recording.")
    print(f"raw log:     {log_path}")
    print(f"trial sheet: {sheet_path}  (fill in buzzer_YN and notes by hand)")
    for r in rows:
        det = "FALL_DETECTED" in r[4]
        print(f"  trial {r[0]:2d} {r[1]:<15} detected={'YES' if det else 'no '}  states={r[4]}")


if __name__ == "__main__":
    main()
