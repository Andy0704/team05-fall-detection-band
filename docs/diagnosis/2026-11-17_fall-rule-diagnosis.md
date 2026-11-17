# Fall-rule diagnosis — pilot sessions 2 and 3 (2026-11-17)

Team 05 fall-detection band. Why the on-board rule detected **0 of 16
pilot falls**, what has been fixed, what is still open, and what the data
can and cannot tell us yet.

All numbers below were measured from committed raw logs with the scripts
named next to them. Both sessions are **tuning-set data**; none of it may
be reused as held-out results.

## 1. Data used

| Session | Raw log | Firmware | Trials | Rate / drops |
|---|---|---|---|---|
| Standing calibration | `data/raw/2026-11-17_onbody-pilot/2026-11-17_standing-still_axis-check.log`, `..._axis-verify.log` | ff6ad6a, 3514471 | standing still 15 s | 100.0000 Hz, 0 gaps |
| Session 2 | `data/raw/2026-11-17_fall-forward_session2.log` + `_trials.csv` | 3954940 (rule v1) | back, right, left, front, down-in-place ×2 | 252.6 s, 100.0000 Hz, 0 gaps |
| Session 3 | `data/raw/2026-11-17_falls_session3.log` + `_trials.csv` | 4bd243c (rule v2) | back, right, left, front ×2, sit-down, lie-down | 410.2 s, 100.0000 Hz, 0 dropped ticks |

- Session 2 labels come from the operators after the session (the recorder
  was run with one label for all loops):
  `data/processed/2026-11-17_fall-session2_labels.csv`.
- Session 3 labels were fixed before recording.
- Scripts: `tools/analyze_log.py` (rate, drops), `tools/analyze_trials.py`
  (per-trial features), `tools/simulate_rule.py` (Python copy of the rule;
  reproduces the board's logged state on 25262/25262 samples of session 2
  with `--rule v1` and 41023/41023 of session 3 with the default `v2`).
- Derived outputs: `data/processed/2026-11-17_fall-session2_trial-diagnostics.txt`,
  `data/processed/2026-11-17_falls-session3_trial-diagnostics.txt`,
  `data/processed/2026-11-17_falls-session3_rule-whatif.txt`.

**Participant:** one healthy team member. **Surface:** mat, with spotter.

## 2. Result on the board

| Session | Falls detected | False alarms | Furthest stage reached by falls |
|---|---|---|---|
| 2 (rule v1) | 0 / 8 | 0 / 2 (down-in-place) | FREE_FALL (one IMPACT_WAIT) |
| 3 (rule v2) | 0 / 8 | 0 / 2 (sit-down, lie-down) | FREE_FALL (one IMPACT_WAIT) |

## 3. Session 3 per-trial features

Landing = largest SVM peak in the trial window (`tools/analyze_trials.py`).
"Free-fall" = longest run with SVM < 0.4 g in the 1 s before the peak.
"Tilt lying" = median over the last 3 s before the get-up cue, a_v = −ax.

| # | Activity | Peak SVM (g) | Free-fall (ms) | Min SVM before impact (g) | Tilt lying (°) | Lying axis |
|---|---|---|---|---|---|---|
| 1 | fall-backward | 6.09 | 20 | 0.32 | 98.5 | +z |
| 2 | fall-right | 6.18 | 40 | 0.27 | 90.6 | +y |
| 3 | fall-left | 4.44 | 10 | 0.36 | 94.2 | −y |
| 4 | fall-forward | 1.96 | 40 | 0.30 | 64.4 | −z |
| 5 | fall-backward | 3.77 | 0 | 0.60 | 95.7 | +z |
| 6 | fall-right | 5.78 | 80 | 0.26 | 95.5 | +y |
| 7 | fall-left | 3.52 | 0 | 0.47 | 90.4 | −y |
| 8 | fall-forward | 4.69 | 50 | 0.06 | 56.4 | −z |
| 9 | sit-down | 3.21 | 0 | 0.74 | 41.9 | −x |
| 10 | lie-down | 4.78 | 0 | 0.64 | 95.9 | +z |

Rule thresholds at the time: free-fall < 0.4 g for ≥ 50 ms → impact
> 2.5 g within 0.5 s → settle 1 s → still (1 ± 0.15 g) for 2 s within 6 s
of impact → tilt > 60°.

## 4. Root causes

| # | Cause | Type | Evidence | Status |
|---|---|---|---|---|
| 1 | `INACTIVITY_CHECK` seeded its min/max with the impact SVM (> 2.5 g), so the still test failed on the next sample: **FALL_DETECTED was unreachable** | Bug | code review of rule v1; simulator | **Fixed** in 4bd243c (SETTLE state, retrying still window) |
| 2 | **Free-fall stage expects true free-fall** (< 0.4 g for ≥ 50 ms). Pilot falls dipped only briefly (0–80 ms) and often only to 0.3–0.6 g. The 44 Hz DLPF further smooths short dips. 0.4 g is a handout value, not measured for a waist sensor | Rule design / threshold | table above, column "Free-fall" | **Open — main blocker** |
| 3 | Tilt used the raw −ax axis; the sensor sits 15.6° off vertical toward +z, which adds tilt lying on the back and removes it face-down (forward 56–64° vs back 96–98°) | Calibration | standing logs; tilt table | Fix proposed (§6 A) |
| 4 | Impact threshold alone does not separate falls from controls: mat falls as low as 1.96 g, sit-down 3.21 g and lie-down 4.78 g exceed 2.5 g | Threshold | column "Peak SVM" | Needs tuning data |
| 5 | Small adjustments after landing used up the 6 s still window (trial 5) | Timing | simulator trace | Needs tuning data |
| 6 | Session 2: wearer did not lie fully down (lying tilt 24–65°). Session 3: landing key pressed 1.5–14 s after the real impact | Protocol | operator report; `sync_ok=no` on all session 3 trials | Fixed: protocol requires lying fully down; analysis anchors on the SVM peak |

## 5. Why the pilot falls look this way — test conditions

These conditions explain cause 2 and part of cause 4, and must be stated as
limitations whenever these data are reported.

1. **Tethered to the PC.** The board is powered and logged over a USB cable
   to the laptop. The faller has to keep the cable from snagging or pulling
   out, which slows and controls the fall.
2. **Fear of falling freely.** The faller is confident doing a fast
   sit-down but not a fast uncontrolled fall. So the falls are guided
   descents (bending, reaching out, lowering) rather than a free drop:
   short, shallow SVM dips before impact.
3. **Soft mat and spotter** reduce the impact peak (CLAUDE.md limitation).
4. **One young, healthy participant.** Not the target population (older
   adults), whose real falls are unplanned.

Consequences:

- The pilot falls are **harder to detect than real falls** in the
  free-fall stage, and **look more like the controls** (a fast sit-down
  has a similar impact peak).
- Tuning the pre-impact threshold to these cautious falls pushes it toward
  the controls, which risks false alarms in daily life. Any threshold
  chosen from these data must be reported together with this limitation.
- More trials *under the same conditions* will not remove this bias; they
  only make the measured numbers more precise.

## 6. Improvements

### A. Tilt relative to the wearer's own standing posture (cause 3)

Use the measured standing gravity vector as the "upright" reference
instead of −ax, and compute tilt as the angle between the current
acceleration and that reference. Reference = mean of
`2026-11-17_standing-still_axis-verify.log`: (−0.932, 0.061, 0.269) g.

Check on session 3 (lying posture, same windows as §3):

| | Tilt vs −ax | Tilt vs standing reference |
|---|---|---|
| Falls (8) | 56–98° | **70–89°** |
| Sit-down | 42° | **26°** |
| Lie-down | 96° | 80° |

All falls clear 60° with margin and the sit-down moves further below it.
This is a calibration from a measured log, not threshold tuning. The
reference must be re-measured if the band's mounting changes.

### B. "Recent low point" instead of free-fall duration (cause 2)

Remember the time of the last sample with SVM below a threshold; at impact,
require that time to be within about 1 s. One timestamp, no buffer.
Session 3 minimum SVM before impact: falls 0.06–0.60 g, sit-down 0.74 g,
lie-down 0.64 g. Promising, but backward fall 5 (0.60) and the lie-down
(0.64) are close, so the threshold must come from the full tuning set.

### C. Impact threshold and still-window timeout (causes 4, 5)

Simulation only (`tools/simulate_rule.py`, not in firmware):

| Variant | Falls | False alarms |
|---|---|---|
| Firmware (4bd243c) | 0 / 8 | 0 / 2 |
| No free-fall stage | 5 / 8 | 0 / 2 |
| No free-fall stage + 10 s timeout | 6 / 8 | **1 / 2** (lie-down) |

Removing the pre-impact check makes a deliberate lie-down look like a fall,
so some pre-impact feature (B) must stay. Lower impact threshold and longer
timeout only make sense together with B, and are tuning decisions.

### D. Make the falls more realistic (section 5)

- **Untether the board:** run it from the power bank (CLAUDE.md: availability
  TBD with a TA) and log during the trial to on-board memory (8 MB PSRAM
  holds roughly an hour at 100 Hz), then dump the log over USB afterwards.
  The fall decision stays on the board (spec 02); only the logging moves.
  Until then: a long, light USB cable routed along the spine with strain
  relief at the waist.
- **Build confidence gradually:** practise on a thicker crash mat, start
  from kneeling and progress to standing, with the spotter. Record which
  starting height each trial used.
- **More than one faller,** so the results are not one person's style.

## 7. Does more data help?

- **Not for causes 1–3 and 6.** These are design/calibration/protocol
  problems that appeared in every trial; more trials of the same kind would
  repeat them.
- **Yes for B and C.** Their thresholds must be chosen on about two thirds of
  the planned set (30 falls, 20 sit-downs, 10 lie-downs, ideally several
  people) and then checked on the untouched third. Picking them from these
  18 trials would overfit.
- **Not for the realism bias (section 5)** unless the test conditions change (D).

## 8. Proposed next steps (pending team approval for rule changes)

1. Implement A (standing-reference tilt) and the structure of B with
   "first guess" thresholds; verify in the simulator against sessions 2–3.
2. Improve test conditions (D): untethered logging or cable management, and
   gradual progression.
3. Record the full tuning set with `tools/record_trials.py`, labels fixed
   before recording, assignment to tuning / held-out fixed before recording.
4. Tune B and C in the simulator on the tuning set only; flash; run the
   held-out trials on the board and report the board's decisions with counts.
