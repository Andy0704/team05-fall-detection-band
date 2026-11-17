# Project status: Team 05 Fall-Detection Band

Last updated: 2026-11-09. Update this file after every work session.

## Summary

End of session state, 2026-11-09: `main` has the sampling task, the
free-fall/impact/tilt/inactivity rule, Joaquin's standalone MPU6050
validation, and (on branch `output-alarm`, not yet merged) a working
buzzer + LCD1602 + cancel-button output stage. The rule is **still not
validated against real fall data** — all thresholds are still "first
guess" or "handout example" — and `a_v` (vertical axis) is still a
placeholder (`az`) pending mounting confirmation. The LCD/button/buzzer
are wired per the pins below but not yet confirmed working by eye/ear
(only a Serial-log sanity check done so far — no crash, 430+ samples
still MONITORING with the output task running). The one genuine
10-minute/100 Hz/0-drop log on record
(`data/raw/2026-11-05_joaquin-standalone-validation_10min_100Hz.log`) is
from a standalone sketch, not the production firmware — spec 04 still
needs its own run on `firmware/src/main.cpp`.

**Resume here next session:**
1. **Buzzer and cancel button both confirmed working** standalone
   (PWM test heard clearly - passive buzzer; button test showed clean
   HIGH/LOW transitions on press/release). No firmware changes needed
   for either - wiring and code are both correct. **The buzzer alone
   already satisfies spec 03's output requirement**, so the project is
   not blocked on the LCD for that spec.
2. **LCD still blank** — see "LCD: repair steps and fallback plan"
   below for the multimeter checks to try next and the fallback
   options if it can't be revived.
3. Real fall-test trials not started yet — see "Real fall-test
   protocol" below for the full step-by-step (calibrating `a_v`,
   running the Test plan tracker trials, splitting before tuning,
   computing sensitivity/specificity).
4. Still need a real 10-minute log with drop rate from `firmware/src/main.cpp`
   itself for spec 04 (only the standalone-sketch log exists so far).

## Repository and collaboration

- GitHub (private): https://github.com/Andy0704/team05-fall-detection-band
- Branch strategy: `main` plus one feature branch per work area — `sensor-sampling`, `detection-rule`, `output-alarm`, `testing`. Work on your branch, open a PR into `main` to integrate; review and resolve conflicts in the PR rather than pushing straight to `main`.
- Collaborators invited 2026-11-05 (pending acceptance): `Joaquinramirezhq`, `f94125032`, `f94127026-maker`.
- LF line endings enforced repo-wide via `.gitattributes` to avoid cross-OS diff noise; `data/raw/**` is excluded from that normalization so raw logs stay byte-exact.
- PR history: #1 closed (superseded by #3), #3 merged (Joaquin's validation, reorganized), #2 merged (detection rule, after a review-fixed sensor-gap bug).

## Deadlines

| Date | Deliverable | Status |
|---|---|---|
| Thu 5 Nov | Proposal (Word + PDF) | Revised draft ready. To do: team confirms, rename to `Team05_Proposal.docx`, export PDF with the same name, upload to LMS |
| Thu 12 Nov | Internal checkpoint (not a course deadline) | Planned: stable sampling and logging at a measured rate near 100 Hz, first 10-minute log committed with its drop rate, labelled trials started |
| Thu 19 Nov | Progress deliverable (PowerPoint) | Not started |
| Thu 3 Dec | Demo Day (Word + PowerPoint) | Not started |

## Spec evidence tracker (needed for 19 Nov)

| Spec | Evidence needed | Status | Measured value | Evidence file |
|---|---|---|---|---|
| 01 Rate | Measured rate in Hz from a real run, shown on screen | In progress: 5 s smoke test done, needs on-device display (not Serial) and a longer run | **100.0000 Hz** (5.1 s smoke test) | `data/raw/2026-11-05_sampling-task-smoketest.log` via `tools/analyze_log.py` |
| 02 Real time | Decision made on the board while the signal arrives | In progress: `FallDetector::update()` now runs in the sampling task on every sample, on the board | n/a | `firmware/src/fall_detector.cpp` |
| 03 Rule | One line of code: named constant with unit | Implemented: free-fall -> impact (within window) -> trunk tilt + inactivity, all named constants in `firmware/include/config.h`. Not yet validated against real fall data (thresholds still "first guess"/"handout example") | idle sanity check: 680/681 samples MONITORING, 1 brief FREE_FALL from handling, 0 FALL_DETECTED | `data/raw/2026-11-05_detection-rule-idle-sanity.log`, summarized in `data/processed/2026-11-05_detection-rule-idle-sanity.txt` |
| 03 Output | Buzzer / LCD reacts with no laptop | Implemented on `output-alarm` branch (not merged). Buzzer (PWM, passive) and cancel button both confirmed working standalone. **LCD still blank, unresolved** — see "Resume here" above. Not yet seen end to end (real alarm trigger -> LCD text + buzzer + button cancel together) | buzzer/button: confirmed by ear/hand; LCD: not yet (still blank) | `firmware/src/main.cpp` |
| 04 Ten min | Committed raw log and drop rate from it | Preliminary evidence only: Joaquin's standalone validation sketch (not the team's FreeRTOS firmware) ran 10 min with 0 drops. The production firmware (`firmware/src/main.cpp`) still needs its own 10-minute run before this spec is actually satisfied | **100.0000 Hz, 0.000000% drop rate** (611.63 s, standalone sketch) | `data/raw/2026-11-05_joaquin-standalone-validation_10min_100Hz.log` via `tools/analyze_log.py`, independently re-verified; see `data/processed/2026-11-05_joaquin-standalone-validation_10min_100Hz_rate.txt` |

## Decisions made (from the revised proposal)

- Waist (lower-back) mounting, one axis along the body's vertical axis.
- MPU6050 at ±8 g, sampled at a 100 Hz target (10 ms), actual rate always measured with `micros()`.
- Sampling in a dedicated high-priority FreeRTOS task with `vTaskDelayUntil()`. Display and logging in lower-priority tasks via a queue.
- Rule: free-fall → impact → trunk tilt + inactivity. Alarm: buzzer + LCD1602 "FALL DETECTED", cancel button, 30 s timeout (first guess).
- Sensor-failure indication: "SENSOR ERROR" on the LCD and two short beeps every 5 s (not implemented yet).
- Logging: `counter,micros,ax,ay,az,state` with a `#` header; counter increments per scheduled tick.
- Test plan: tune on about two thirds of the trials, report on the held-out third.

## Open decisions and items to confirm

1. Team agreement on waist mounting and the 60° tilt condition (proposed, not yet agreed by all members). **Blocking a real parameter now:** `firmware/src/fall_detector.cpp` assumes `a_v` (vertical axis) is `az` as a placeholder — confirm the real mounting orientation once the band is actually worn and update `verticalComponentG()` if a different axis is vertical.
2. ~~Kit contents: OLED, buzzer, push button.~~ Resolved 2026-11-09: have a QAPASS 1602A LCD (parallel, no I2C), a 4-pin button, and a small buzzer (active/passive not confirmed from the part alone, driven via PWM either way). Still TBD: power bank, belt / strap — ask a TA.
3. ~~IMU detected on I2C at 0x68 (WHO_AM_I read)?~~ Verified 2026-11-05 on hardware: `firmware/src/main.cpp` flashed to the ESP32-S3-N16R8, MPU6050 data-ready interrupt on GPIO4 fires and `WHO_AM_I` reads back `0x68` continuously. One transient `i2cWriteReadNonStop Error -1` seen in roughly 80 reads over ~5 s — not yet characterized, watch for recurrence once the sampling task is running.
4. ~~OLED on the shared I2C bus, a second I2C controller, or MPU6050 FIFO.~~ Moot: the LCD1602 in hand is parallel (no I2C), so it never shares the I2C bus with the MPU6050.
5. DLPF setting (first choice 44 Hz) and final accelerometer range.
6. ~~Toolchain (Arduino IDE or PlatformIO) and repo layout.~~ Decided: PlatformIO, project at `firmware/`, board `esp32-s3-devkitc-1` (chip variant ESP32-S3-N16R8).
7. Roles: the proposal form has no roles section. Suggested split: sensor and sampling, detection rule, output, testing and measurement. Everyone must understand the whole project.
8. Test logistics: mat, spotter, who performs the falls, where. The step-by-step procedure is now written (see "Real fall-test protocol" below); still need to actually schedule it — book a mat, confirm who's the faller/spotter/logger, and pick a location/date.

~~9. MPU6050 INT pin wiring.~~ Resolved 2026-11-05: SDA→GPIO8, SCL→GPIO9, INT→GPIO4, VCC→3V3, GND→GND. Matches `firmware/include/pins.h` and CLAUDE.md Hardware section, no code change needed. Still need to flash and confirm on hardware (item 3).

## LCD: repair steps and fallback plan

Blank (backlight on, no characters) since first wired 2026-11-09. Software, V0 (contrast), RW, and VSS/VDD power have all been checked and fixed in turn without success — see the Log entries below for the full trail. This section is the plan for whoever picks it up next.

### Repair steps to try (in order)

1. **Multimeter at the LCD's own pins, not the power rail.** Measure VSS (pin 1) to VDD (pin 2) directly on the LCD's 16-pin header. Expect close to 5 V. If it reads near 0 V or is noisy/unstable, the power isn't actually reaching the chip despite the rail itself being fine — look for a loose jumper or a dead breadboard contact between the rail and the LCD.
2. **Continuity test (multimeter beep mode).** Confirm each of RS, EN, D4, D5, D6, D7 (LCD pins 4, 6, 11, 12, 13, 14) has an unbroken connection to GPIO 5, 6, 7, 15, 16, 17 respectively, and that no two of those lines are accidentally shorted to each other or to an adjacent breadboard row.
3. **Move the LCD to a different breadboard row and swap jumper wires one at a time.** Old breadboards can have worn-out contacts in specific rows; a jumper that looks fine can have a broken conductor inside.
4. If all of the above check out clean and it's still blank, the unit itself may simply be faulty after 10+ years unused — a real possibility, just not the first thing to suspect, since a floating RW/VSS/VDD already explained every symptom seen so far (blank screen, irregular flashing on insertion) without needing to invoke age.

### Fallback plan if it can't be revived

**The buzzer alone already satisfies spec 03's on-device output requirement** (CLAUDE.md "four mandatory specs" lists the buzzer as a valid output by itself) — a working LCD is not required to pass that spec, which lowers the urgency here.

- **Option A (minimum effort):** ship without the LCD. Buzzer + cancel button is a complete, spec-compliant alarm on its own. For the team's own visibility during development, read the Serial log on a laptop — that's for debugging only (CLAUDE.md code rule 4), not counted as "the output" for spec 03 either way.
- **Option B:** ask a TA for a replacement LCD-1602A unit, or borrow one from another team. Retest with the exact same firmware — no code changes needed if the replacement is pin-compatible, since `firmware/include/pins.h` already has the right GPIO assignments.
- **Option C:** swap to an I2C OLED (what the original proposal assumed before this LCD was in hand). Frees GPIO 5/6/7/15/16/17 back up for other uses. Needs: a new display library (e.g. `Adafruit_SSD1306` + `Adafruit_GFX`), rewriting `outputTask`'s LCD calls in `firmware/src/main.cpp`, and re-reading the I2C-contention note in CLAUDE.md's Design decisions (it would become relevant again, since an I2C OLED shares the bus with the MPU6050).
- Whichever option is chosen, update CLAUDE.md's Hardware/Design-decisions sections and this file's Summary to describe only the display actually in use — don't leave the old and new display both described at once.

## Real fall-test protocol

Not started yet. Safety first (CLAUDE.md): **falls only onto a mat, with a spotter, never from height. Stop immediately if anyone feels unsafe.**

### 0. Before the first fall

1. Flash the latest `output-alarm` firmware (or `main`, once that branch is merged).
2. Put the band on the person who will do the fall trials, worn at the waist as actually intended for use (not just resting on a desk).
3. With them standing still, read the Serial log's `ax,ay,az` for about 5 s and note which axis sits closest to ±1 g — that is the real `a_v` for this mounting. If it is not `az`, update `verticalComponentG()` in `firmware/src/fall_detector.cpp` *before* running any trials, and record the change here with today's date and the measured values (this resolves open decision #1 with real data instead of the current placeholder).
4. Confirm roles: the **faller** (healthy team member, able to bail out safely), the **spotter** (physically supports/catches, never lets the faller hit from height), and whoever **watches the Serial log** / operates the laptop and the cancel button.

### 1. Run the trials

Use the categories and counts in the Test plan tracker below. For each individual trial:

1. Confirm the wearer is in a clear `MONITORING` state (Serial log, and LCD once it works) before starting.
2. Perform the action (fall, sit-down, jump, etc.).
3. Watch for `FALL_DETECTED` within a few seconds (Serial log `state` column; LCD text + buzzer once the LCD works).
4. Record: trial number, category, expected result, actual result (alarm / no alarm), and roughly where in the log it happened (counter or `micros` value).
5. Press the cancel button to clear the alarm before the next trial.
6. Save the full session's raw Serial log to `data/raw/` with a descriptive name (e.g. `data/raw/2026-11-16_fall-trials-session1.log`) — capture the whole session in one file rather than per-trial snippets, and never edit it afterward.

### 2. Split before tuning (CLAUDE.md honesty rule)

- Use about two thirds of each category's trials to look at the actual measured SVM/tilt/duration values during real falls vs. real non-falls, and adjust thresholds in `firmware/include/config.h` using that data. Cite the specific log and trial numbers that justified each change, and update the Threshold register below (new value, source = the log file, Validated = yes).
- Hold out the remaining third completely untouched during tuning. Run those only after threshold tuning is finished, and report their results as measured, even if worse than the tuning set.

### 3. Compute and report

- Sensitivity = detected falls ÷ all fall trials. Specificity = non-fall trials without an alarm ÷ all non-fall trials.
- Report with counts per activity, e.g. "9/10 falls detected, 2 false alarms in 20 sit-downs" — never a bare percentage with no counts.
- Cite the raw log file(s) and the script that computed the numbers (extend `tools/analyze_log.py`, or write a new script under `tools/`, rather than computing by hand).
- State limitations explicitly: mat landings are softer than a real floor (impact peak likely underestimated); participants are healthy team members, not the band's actual target population (older adults at fall risk).

## Threshold register

| Constant | Value | Unit | Source | Validated |
|---|---|---|---|---|
| `FREEFALL_THRESHOLD_G` | 0.4 | g | handout example | no |
| `FREEFALL_MIN_MS` | 50 | ms | first guess (5 samples at 100 Hz) | no |
| `IMPACT_THRESHOLD_G` | 2.5 | g | handout example | no |
| `IMPACT_WINDOW_S` | 0.5 | s | handout example | no |
| `TILT_THRESHOLD_DEG` | 60 | degrees | first guess | no |
| `INACTIVITY_S` | 2 | s | first guess | no |
| `INACTIVITY_SVM_DEV_G` | 0.15 | g | first guess | no |
| `POST_IMPACT_SETTLE_MS` | 1000 | ms | first guess (added 2026-11-17) | no |
| `INACTIVITY_TIMEOUT_MS` | 6000 | ms | first guess (added 2026-11-17) | no |
| `STANDING_SVM_G` | 1.0 | g | nominal (gravity only) | n/a |
| `ALARM_TIMEOUT_S` | 30 | s | first guess | no |
| `BUTTON_DEBOUNCE_MS` | 30 | ms | first guess | no |
| `DISPLAY_REFRESH_MS` | 1000 | ms | design choice | n/a |
| `OUTPUT_POLL_MS` | 20 | ms | design choice | n/a |
| `BUZZER_PWM_FREQ_HZ` | 2000 | Hz | first guess, not measured against the actual buzzer | no |
| `SENSOR_STALE_MS` | 30 | ms | first guess | no |
| `SENSOR_IDENTICAL_SAMPLES` | 50 | samples | first guess | no |
| `SENSOR_RECOVER_SAMPLES` | 100 | samples | first guess | no |

## Test plan tracker

| Test | Expected | Planned trials | Done | Log files |
|---|---|---|---|---|
| Falls onto a mat, with spotter (forward, backward, lateral) | Fall detected | 30 (3 × 10) | 0 | none |
| Fast sit-down | No alarm | 20 | 0 | none |
| Deliberate lying down on a mat | No alarm | 10 | 0 | none |
| Jumping | No alarm | 10 | 0 | none |
| Walking | No alarm | 4 persons × 2 min | 0 | none |
| Stairs | No alarm | 4 persons × 1 up-and-down run | 0 | none |

## Risks

| Risk | How we check it |
|---|---|
| Fast sit-down looks like a fall in SVM alone | Trunk-tilt condition; compare sit-down, lying-down and fall trials |
| Some falls have no clear free-fall dip | Plot SVM in mat-fall trials; relax the free-fall condition with evidence |
| Sample loss at 100 Hz (IMU, LCD and serial compete for CPU) | Distribution of sampling intervals and counter gaps in the 10-minute log. Lower risk than originally scoped: the LCD1602 is parallel, not I2C, so it cannot contend with the MPU6050 for the I2C bus |
| Mat landings underestimate impact peaks | State as a limitation; compare sit-down peaks on a hard chair |
| Hardware parts missing from the kit | Confirm with a TA now, not on 19 Nov |

## Log

- 2026-11-05: File created from the revised proposal. No measurements recorded yet.
- 2026-11-05: PlatformIO project skeleton created at `firmware/` (board `esp32-s3-devkitc-1`, chip ESP32-S3-N16R8). Minimal I2C scan + MPU6050 WHO_AM_I check written in `firmware/src/main.cpp`, triggered by the MPU6050 data-ready interrupt (not polling). Builds successfully with `pio run`. No sampling task yet.
- 2026-11-05: Wired MPU6050 (SDA→GPIO8, SCL→GPIO9, INT→GPIO4, VCC→3V3, GND→GND), flashed and verified on hardware: WHO_AM_I reads 0x68 continuously via the INT-driven path. One transient I2C error in ~80 reads, noted above for follow-up.
- 2026-11-05: Replaced the diagnostic ISR with the real sampling architecture: a high-priority FreeRTOS task reads the accelerometer (±8 g, DLPF 44 Hz) every 10 ms via `vTaskDelayUntil()`, pushes to a queue consumed by a lower-priority logging task (`firmware/src/main.cpp`). `state` is a placeholder (`MONITORING`) — no fall rule yet. 5.1 s smoke test: measured rate 100.0000 Hz, 0 dropped ticks. Raw log: `data/raw/2026-11-05_sampling-task-smoketest.log`, measured by `tools/analyze_log.py` (output saved to `data/processed/2026-11-05_sampling-task-smoketest_rate.txt`). Note: this measurement predates git init below, so its log header has no commit hash — that gap is now closed for future logs.
- 2026-11-05: `git init`. LF line endings enforced repo-wide via `.gitattributes` (multi-OS team); `data/raw/**` explicitly excluded from any text normalization so raw logs stay byte-exact. Initial history split into docs / firmware skeleton / tools+data commits. Added `firmware/git_version.py` (PlatformIO pre-build script) to inject the current commit hash into the log header automatically. Remote hosting and branch/merge workflow for parallel work with teammates not decided yet.
- 2026-11-05: Created private GitHub repo `Andy0704/team05-fall-detection-band`, pushed `main`, created and pushed feature branches `sensor-sampling`/`detection-rule`/`output-alarm`/`testing`, invited the three teammates as collaborators, and wrote `README.md`.
- 2026-11-05: On `detection-rule` branch: implemented the free-fall -> impact -> trunk tilt + inactivity rule (`firmware/include/fall_detector.h`, `firmware/src/fall_detector.cpp`), wired into the sampling task's `state` field. All thresholds as named constants in `firmware/include/config.h`. `a_v` (vertical axis) is a placeholder (`az`) pending mounting confirmation (open decision #1). Idle sanity check on hardware (board at rest on a desk, no fall performed): 680/681 samples stayed MONITORING, 1 brief FREE_FALL from handling the board, never reached FALL_DETECTED — log in `data/raw/2026-11-05_detection-rule-idle-sanity.log`. Still need: real fall/no-fall trials with a mat and spotter to validate the thresholds (none done yet).
- 2026-11-09: Joaquin uploaded a standalone MPU6050 validation (I2C scanner, accel readings, 100 Hz sampling sketch, and a genuine 10-minute 100 Hz log) via GitHub's web upload (`joaquinramirezhq-patch-1`, PR #1), but the files landed at the repo root, which would have overwritten `README.md` and added a conflicting root `platformio.ini` if merged as-is. Reorganized on branch `integrate-joaquin-validation`: the three `.cpp` sketches, his `platformio.ini`, and his README moved to `experiments/imu-100hz-validation/` (matching his own stated intent); the raw log moved to `data/raw/2026-11-05_joaquin-standalone-validation_10min_100Hz.log` (byte-exact copy) with a derived report in `data/processed/`. Independently re-verified with `tools/analyze_log.py`: 61,164 samples, 611.63 s, 100.0000 Hz, 0 dropped ticks — matches his reported numbers. This is preliminary hardware evidence only (standalone single-loop sketch, not the team's FreeRTOS firmware), so spec 04 is not yet satisfied by the production firmware.
- 2026-11-09: Code review on PR #3 (this integration) found: (1) `experiments/imu-100hz-validation/` had no `.gitignore` for its own `.pio/` build output, unlike `firmware/` — added one. (2) `(Wire.read() << 8) | Wire.read()` in the two copied sketches relies on unspecified `|` operand evaluation order, risking swapped MSB/LSB bytes on a different compiler/optimization level — split into separate statements in both files, and the same latent bug was also found and fixed in `firmware/src/main.cpp` on the `detection-rule` branch (PR #2). (3) The raw log's own header doesn't carry the DLPF/accel-range/position/commit-hash/date that CLAUDE.md code rule 5 asks for — documented as a known limitation in the experiment's README and the processed report (not fixed by editing the raw log, which is never edited or re-saved).
- 2026-11-09: Code review on PR #2 found a real bug: the sampling task only called `FallDetector::update()` on successful reads, so a sensor-read gap mid-sequence froze the phase timer; combined with `IMPACT_WAIT` checking `svm` before checking window expiry, a stale impact after a gap could still be accepted, and `INACTIVITY_CHECK` could confirm `FALL_DETECTED` off a single post-gap sample. Fixed: `update()` now takes a `valid` flag and runs every tick, resetting to `MONITORING` on an invalid read during any active phase; `IMPACT_WAIT` checks window expiry first; tilt computation moved inside the branch where `svm > IMPACT_THRESHOLD_G` is already guaranteed (also structurally rules out the `svm == 0` -> NaN case review flagged); added `STANDING_SVM_G` named constant instead of a bare `1.0f`; fixed the same `(Wire.read() << 8) | Wire.read()` evaluation-order issue in `firmware/src/main.cpp`. Re-ran the idle sanity check after the fix: 536/537 samples MONITORING, no regression.
- 2026-11-09: Received a QAPASS 1602A LCD (parallel HD44780, no I2C backpack, confirmed from photos — plain 16-pin header, no I2C adapter board), a 4-pin button, and a small electromagnetic buzzer (active/passive not determinable from the part alone). On branch `output-alarm` (merged with `main`): wired RS=GPIO5, EN=GPIO6, D4=GPIO7, D5=GPIO15, D6=GPIO16, D7=GPIO17, button=GPIO18, buzzer=GPIO10 (all from the ESP32-S3-DevKitC-1 left header row, skipping pins already used and the strapping pins). Implemented `outputTask` in `firmware/src/main.cpp`: shows "FALL DETECTED" + drives the buzzer via `ledc` PWM (works for either buzzer type) when the detector reaches `FALL_DETECTED`, clears on a debounced button press or `ALARM_TIMEOUT_S`, then calls `FallDetector::reset()` via a shared flag back to the sampling task. Idle status line refreshes on state change or once a second, not every sample (`DISPLAY_REFRESH_MS`). Added `arduino-libraries/LiquidCrystal` as a `platformio.ini` dependency. Builds clean; flashed and ran for 430+ samples with no crash, all MONITORING — but nobody has yet confirmed by eye/ear that the LCD/buzzer/button actually work, since this was only a Serial-log sanity check.
- 2026-11-09: **LCD still blank (backlight on, no characters) — unresolved, parked.** Troubleshooting so far, in order, each one re-tested and still blank: (1) V0 tied to VCC/floating -> moved to GND (max contrast). (2) RW floating -> tied to GND. (3) Read SunFounder's LCD-1602A datasheet (`CN0295D other related document.pdf`, dropped in repo root — not committed, see `.gitignore`): spec requires VDD-VSS = 4.5-5.5V, but VSS/VDD were never wired at all (only the backlight A/K pins were connected, which is why the backlight glows but the logic IC never powers on) -> wired VSS to GND and VDD to the board's 5V pin. Still blank. (4) Confirmed twice, independently, that the firmware is not the problem: added temporary `Serial.println` tracing in `outputTask` showing `lcd.print()` executes every second (stashed, not committed — `git stash list`); then swapped `firmware/src/main.cpp` for a minimal standalone `lcd.begin()`/`lcd.print()` test sketch (also reverted, not committed) — Serial confirms the code runs to completion every time, so this is purely electrical, not software. Next step: measure VSS/VDD voltage directly at the LCD's own header pins (not just at the power rail) with a multimeter, and visually audit RS/EN/D4-D7 wiring against GPIO 5/6/7/15/16/17 pin-by-pin. Paused for now to test the buzzer instead.
- 2026-11-09: **Buzzer confirmed working.** Standalone test (temporarily swapped into `firmware/src/main.cpp`, reverted after, not committed): cycled PWM 2kHz 50% duty (1s) / off (1s) / plain `digitalWrite` HIGH, no PWM (1s) / off (1s), repeating. Clearly loud during the PWM phase, only a faint transient click (pin-reconfiguration artifact, not a real tone) during the plain-DC phase — so it's a **passive buzzer**, consistent with needing the PWM drive `outputTask` already uses (`BUZZER_PWM_FREQ_HZ`/`BUZZER_PWM_DUTY`). No firmware change needed; wiring and code for the buzzer are both correct. Restored the real firmware from git and reflashed; sampling/logging still running fine afterward (counter past 1800, still MONITORING).
- 2026-11-09: **Cancel button confirmed working.** Standalone test (same temporary-swap pattern, reverted after, not committed): `pinMode(BUTTON_PIN, INPUT_PULLUP)`, print on every change. After a hardware reset, immediately saw clean `HIGH (released)` / `LOW (pressed)` transitions on press/release, no stuck state or chatter. Wiring and the `INPUT_PULLUP` assumption in `debouncedButtonPressed()` (`firmware/src/main.cpp`) are both correct. Restored the real firmware and reflashed. Button cancel's full behavior (clearing `FALL_DETECTED` and calling `FallDetector::reset()`) still needs an end-to-end check once a real alarm can be triggered.
- 2026-11-17: Moved `platformio.ini` to the repo root (`src_dir`/`include_dir` -> `firmware/`) so PIO Home detects the project; builds and uploads unchanged. **On-body axis check (sitting straight, not yet standing):** band worn at the waist, wearer seated upright and still, 15 s capture after reset. Post-reset part: 1477 samples, counter 1->1477, 14.760 s, 100.0000 Hz, 0 gaps, 0 invalid reads, all MONITORING. Mean ax=-0.715 g, ay=0.177 g, az=0.707 g (SVM 1.02 g): gravity sits about 45 deg between -ax and +az, so **no single axis is vertical in this mounting** — with the `az` placeholder the computed tilt while sitting upright is already ~46 deg (threshold 60 deg). Open decision #1 still open: re-mount the sensor so one axis is aligned with the trunk, then repeat standing. Raw log: `data/raw/2026-11-17_onbody-pilot/2026-11-17_sitting-straight_axis-check.log` (firmware ff6ad6a; its first line is a partial line from the run before the reset — `tools/analyze_log.py` miscounts it as a gap because the counter restarts; numbers above computed on the post-reset section only).
- 2026-11-17: **On-body axis check, standing still — resolves open decision #1.** Same wearer, band at the waist, standing still, 15 s capture after reset: 1477 samples, counter 1->1477, 14.760 s, 100.0000 Hz, 0 gaps, 0 invalid, all MONITORING. Mean ax=-0.936 g (sd 0.004), ay=0.044 g, az=0.258 g, SVM 0.97 g: gravity is along **-ax** (15.6 deg off), az is 74.6 deg off. The old `az` placeholder would have reported ~75 deg tilt while standing upright (above the 60 deg threshold). Changed `verticalComponentG()` in `firmware/src/fall_detector.cpp` to return `-ax`; standing tilt is now ~16 deg. Rebuilt and flashed (change not yet committed). Raw log: `data/raw/2026-11-17_onbody-pilot/2026-11-17_standing-still_axis-check.log` (recorded on ff6ad6a, before the axis change). The earlier sitting log's 45 deg split was the seated posture tilting the pelvis, not the mounting.
- 2026-11-17: Verified the -ax change on hardware (firmware 3514471): standing still 14.75 s, 1476 samples, 100.0000 Hz, 0 gaps, 0 invalid, all MONITORING. Tilt from -ax (computed offline from the logged ax/ay/az): mean 16.6 deg, range 11.5-22.6 deg, well under TILT_THRESHOLD_DEG = 60. Raw log: `data/raw/2026-11-17_onbody-pilot/2026-11-17_standing-still_axis-verify.log`. Next: confirm faller/spotter/laptop roles and a mat, then start the Test plan trials (protocol step 1).
- 2026-11-17: **First guided fall session (pilot, tuning set)** with `tools/record_trials.py`, firmware 3954940. 10 loops; actual order reported by the operators after the session (the recorder was run with the label `fall-forward` for all, so the raw filename/sheet label is wrong): back, right, left, front, down-in-place, back, right, left, front, down-in-place — stored in `data/processed/2026-11-17_fall-session2_labels.csv`. Operators did not always act on the on-screen cue, so `tools/analyze_trials.py` analyses each whole 23 s loop window. **Result: 0/8 falls detected, 0/2 down-in-place alarms; only trial 7 reached IMPACT_WAIT.** Findings (`data/processed/2026-11-17_fall-session2_trial-diagnostics.txt`): (1) longest free-fall run before the peak was 0-40 ms in every trial, below FREEFALL_MIN_MS = 50 ms — mat falls from standing had no clear free-fall dip (Risk table item). (2) Peak SVM 1.92-5.96 g; both backward falls stayed under 2.5 g. (3) Tilt is evaluated at the impact sample (21-62 deg), not after landing. (4) **Bug: INACTIVITY_CHECK seeds svm_min/svm_max with the impact SVM (> 2.5 g), so `stillMoving` is true on the very next sample and FALL_DETECTED is unreachable.** No threshold changes made; rule fix needs team agreement. Raw: `data/raw/2026-11-17_fall-forward_session2.log` + `_trials.csv`. An earlier attempt (`2026-11-17_fall-forward_session1.log`, ~122 s, stopped by the user, no sheet) is left uncommitted pending a keep/delete decision.
- 2026-11-17: **Rule fix (Part 1), approved temporarily by the team.** `firmware/src/fall_detector.cpp`: after impact a new `SETTLE` state ignores motion for `POST_IMPACT_SETTLE_MS` (1000 ms); `INACTIVITY_CHECK` restarts its still window on movement instead of aborting, gives up after `INACTIVITY_TIMEOUT_MS` (6000 ms) from impact; tilt is now checked at the end of the still window (lying posture) instead of at the impact sample. Free-fall/impact thresholds unchanged. Added `tools/simulate_rule.py` (Python copy of the rule, `--rule v1` = old rule): on session 2 it matches the board's logged state on 25262/25262 samples. New rule on session 2 with current thresholds: still 0/8 falls (all blocked at free-fall, <= 40 ms vs 50 ms). What-if with free-fall disabled (tuning-set exploration only, not adopted): 2/8 falls detected (both left falls, where the wearer lay fully down, lying tilt 84 deg), 0/2 down-in-place alarms; the others failed on tilt because the wearer did not lie fully down. Flashed and idle-checked on the board (15 s, 1479 samples, all MONITORING, simulator agrees 100%). `firmware/git_version.py` now appends `-dirty` (or `-unverified` when git cannot tell) to the logged commit hash. Git was briefly blocked because iCloud had offloaded files under `.git/` (Documents folder is iCloud-synced); refs/objects re-downloaded, the index was rebuilt from HEAD (old placeholder kept as `.git/index.icloud-dataless.bak`). Consider moving the repo out of iCloud-synced `Documents`. Next: protocol must require lying fully down; decide free-fall handling with the bigger tuning set.
- 2026-11-17: **Recorder rewritten for robust sync** (`tools/record_trials.py`): operator-paced instead of fixed timers — SPACE when the faller is ready, 5 s stand, spoken "fall", SPACE the moment the body lands, 12 s lie (faller must lie FULLY down), spoken "cancel, get up". Every key/phase is stamped with the board's live counter. After the session each trial is checked against the raw data: the largest SVM peak between the fall cue and landing + 2 s must be within 1 s of the landing key (`sync_ok` = yes / no / no_impact). `s` skips a trial (noted), `q` stops. Firmware and log format unchanged. Dry-run (scripted keys, nobody falling) on 4bd243c: phases and counters recorded correctly, both trials correctly flagged as no real landing.
- 2026-11-17: **Fall session 3 (tuning set)**, operator-paced recorder, firmware 4bd243c (rule v2). 10 trials in pre-set order: back, right, left, front, back, right, left, front, sit-down, lie-down (the last two started from standing, no fall). 410.2 s, 100.0000 Hz measured, 0 dropped ticks (`tools/analyze_log.py`). **Board result: 0/8 falls detected, 0/2 false alarms; all falls stopped at the free-fall stage** (runs 0-80 ms; only trial 6 reached IMPACT_WAIT). Simulator matches the board on 41023/41023 samples. Sync: every landing key press came 1.5-14 s after the real SVM peak (`sync_ok=no` on all 10), so analysis anchors the landing on the SVM peak, with key presses only bounding the window. Wearer lay fully down this time: lying tilt 90-98 deg for back/side falls, 56-64 deg for forward falls (prone), 42 deg sit-down, 96 deg lie-down. Lying axis is consistent per direction: back +z, front -z, right +y, left -y. Peak SVM: falls 1.96-6.18 g (one forward fall under 2.5 g); sit-down 3.21 g and lie-down 4.78 g also exceed IMPACT_THRESHOLD_G. What-if (simulation only, not adopted): with the free-fall stage removed, 5/8 falls detected, 0/2 false alarms; with that plus INACTIVITY_TIMEOUT_MS 10 s, 6/8 falls but the deliberate lie-down becomes a false alarm — so free-fall (or another pre-impact feature) is what separates falls from lying down, and needs tuning on more data. Minimum SVM in the 1 s before impact: falls 0.06-0.60 g vs sit 0.74 / lie 0.64 g (small n, a candidate feature). Files: `data/raw/2026-11-17_falls_session3.log` + `_trials.csv`, `data/processed/2026-11-17_falls-session3_trial-diagnostics.txt`, `..._rule-whatif.txt`.
- 2026-11-09: Wrote two new planning sections: "LCD: repair steps and fallback plan" (multimeter/continuity checks to try next, plus three fallback options — ship buzzer-only, get a replacement LCD-1602A, or swap to an I2C OLED — each with the explicit note that the buzzer alone already satisfies spec 03) and "Real fall-test protocol" (role assignment, calibrating `a_v` from a real standing measurement before any trial, the two-thirds/one-third tuning split, and how to compute and report sensitivity/specificity). Also fixed `CLAUDE.md`'s repo-layout entry for the skill file (it's `SKILL.md` at the repo root, not `.claude/skills/fall-band-spec-check/SKILL.md`, which never existed) and replaced the remaining "OLED" wording in `SKILL.md` with "LCD".
