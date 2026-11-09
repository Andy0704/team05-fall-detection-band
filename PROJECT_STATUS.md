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
1. On branch `output-alarm` (merged with `main`), builds and boots clean
   with the LCD/button/buzzer wired. **LCD is still blank** after fixing
   V0, RW, and VSS/VDD wiring (see Log below) — confirmed the firmware
   itself is fine (runs to completion every time, even with a minimal
   standalone test sketch), so this is an electrical/wiring issue not
   yet found. Next: measure VSS/VDD at the LCD's own pins with a
   multimeter, audit RS/EN/D4-D7 wiring pin-by-pin. Buzzer and cancel
   button not yet confirmed working either way.
2. Then: real fall/no-fall trials with a mat and spotter to validate the
   thresholds and confirm the `a_v` axis (open decision #1) — see "Open
   decisions" below.
3. Still need a real 10-minute log with drop rate from `firmware/src/main.cpp`
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
| 03 Output | Buzzer / LCD reacts with no laptop | Implemented on `output-alarm` branch (not merged): `outputTask` in `firmware/src/main.cpp` shows "FALL DETECTED" on the LCD1602 and drives the buzzer via PWM on `FALL_DETECTED`, clears on button press or `ALARM_TIMEOUT_S`. Not yet confirmed working by eye/ear — only a Serial sanity check (no crash) so far | n/a (not yet observed in person) | `firmware/src/main.cpp` |
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
8. Test logistics: mat, spotter, who performs the falls, where.

~~9. MPU6050 INT pin wiring.~~ Resolved 2026-11-05: SDA→GPIO8, SCL→GPIO9, INT→GPIO4, VCC→3V3, GND→GND. Matches `firmware/include/pins.h` and CLAUDE.md Hardware section, no code change needed. Still need to flash and confirm on hardware (item 3).

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
