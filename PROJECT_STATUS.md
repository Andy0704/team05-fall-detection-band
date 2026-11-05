# Project status: Team 05 Fall-Detection Band

Last updated: 2026-11-05. Update this file after every work session.

## Summary

The revised proposal is written and ready for the team to confirm and submit today (5 Nov, Word + PDF). Nothing has been measured yet. Hardware and firmware progress is not recorded here yet, so items below marked "unverified" must be checked before anyone relies on them.

Next steps: confirm the open decisions, get the IMU streaming at a measured rate, then the first 10-minute log by the 12 Nov checkpoint.

## Repository and collaboration

- GitHub (private): https://github.com/Andy0704/team05-fall-detection-band
- Branch strategy: `main` plus one feature branch per work area — `sensor-sampling`, `detection-rule`, `output-alarm`, `testing`. Work on your branch, open a PR into `main` to integrate; review and resolve conflicts in the PR rather than pushing straight to `main`.
- Collaborators invited 2026-11-05 (pending acceptance): `Joaquinramirezhq`, `f94125032`, `f94127026-maker`.
- LF line endings enforced repo-wide via `.gitattributes` to avoid cross-OS diff noise; `data/raw/**` is excluded from that normalization so raw logs stay byte-exact.

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
| 02 Real time | Decision made on the board while the signal arrives | Not started (sampling task only reads+logs so far, no decision rule yet) | n/a | none |
| 03 Rule | One line of code: named constant with unit | Not started | n/a | none |
| 03 Output | Buzzer / OLED reacts with no laptop | Not started | n/a | none |
| 04 Ten min | Committed raw log and drop rate from it | Not started (only a 5 s smoke test so far, drop rate 0.0% over that window) | not measured yet (10 min) | none |

## Decisions made (from the revised proposal)

- Waist (lower-back) mounting, one axis along the body's vertical axis.
- MPU6050 at ±8 g, sampled at a 100 Hz target (10 ms), actual rate always measured with `micros()`.
- Sampling in a dedicated high-priority FreeRTOS task with `vTaskDelayUntil()`. Display and logging in lower-priority tasks via a queue.
- Rule: free-fall → impact → trunk tilt + inactivity. Alarm: buzzer + OLED "FALL DETECTED", cancel button, 30 s timeout (first guess).
- Sensor-failure indication: "SENSOR ERROR" on the OLED and two short beeps every 5 s.
- Logging: `counter,micros,ax,ay,az,state` with a `#` header; counter increments per scheduled tick.
- Test plan: tune on about two thirds of the trials, report on the held-out third.

## Open decisions and items to confirm

1. Team agreement on waist mounting and the 60° tilt condition (proposed, not yet agreed by all members).
2. Kit contents: OLED, buzzer, push button, power bank, belt / strap. Ask a TA.
3. ~~IMU detected on I2C at 0x68 (WHO_AM_I read)?~~ Verified 2026-11-05 on hardware: `firmware/src/main.cpp` flashed to the ESP32-S3-N16R8, MPU6050 data-ready interrupt on GPIO4 fires and `WHO_AM_I` reads back `0x68` continuously. One transient `i2cWriteReadNonStop Error -1` seen in roughly 80 reads over ~5 s — not yet characterized, watch for recurrence once the sampling task is running.
4. OLED on the shared I2C bus, a second I2C controller, or MPU6050 FIFO. Decide after measuring the OLED refresh time.
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
| `ALARM_TIMEOUT_S` | 30 | s | first guess | no |
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
| Sample loss at 100 Hz (IMU, OLED and serial compete for CPU and I2C bus) | Distribution of sampling intervals and counter gaps in the 10-minute log; measure OLED refresh time |
| Mat landings underestimate impact peaks | State as a limitation; compare sit-down peaks on a hard chair |
| Hardware parts missing from the kit | Confirm with a TA now, not on 19 Nov |

## Log

- 2026-11-05: File created from the revised proposal. No measurements recorded yet.
- 2026-11-05: PlatformIO project skeleton created at `firmware/` (board `esp32-s3-devkitc-1`, chip ESP32-S3-N16R8). Minimal I2C scan + MPU6050 WHO_AM_I check written in `firmware/src/main.cpp`, triggered by the MPU6050 data-ready interrupt (not polling). Builds successfully with `pio run`. No sampling task yet.
- 2026-11-05: Wired MPU6050 (SDA→GPIO8, SCL→GPIO9, INT→GPIO4, VCC→3V3, GND→GND), flashed and verified on hardware: WHO_AM_I reads 0x68 continuously via the INT-driven path. One transient I2C error in ~80 reads, noted above for follow-up.
- 2026-11-05: Replaced the diagnostic ISR with the real sampling architecture: a high-priority FreeRTOS task reads the accelerometer (±8 g, DLPF 44 Hz) every 10 ms via `vTaskDelayUntil()`, pushes to a queue consumed by a lower-priority logging task (`firmware/src/main.cpp`). `state` is a placeholder (`MONITORING`) — no fall rule yet. 5.1 s smoke test: measured rate 100.0000 Hz, 0 dropped ticks. Raw log: `data/raw/2026-11-05_sampling-task-smoketest.log`, measured by `tools/analyze_log.py` (output saved to `data/processed/2026-11-05_sampling-task-smoketest_rate.txt`). Note: this measurement predates git init below, so its log header has no commit hash — that gap is now closed for future logs.
- 2026-11-05: `git init`. LF line endings enforced repo-wide via `.gitattributes` (multi-OS team); `data/raw/**` explicitly excluded from any text normalization so raw logs stay byte-exact. Initial history split into docs / firmware skeleton / tools+data commits. Added `firmware/git_version.py` (PlatformIO pre-build script) to inject the current commit hash into the log header automatically. Remote hosting and branch/merge workflow for parallel work with teammates not decided yet.
- 2026-11-05: Created private GitHub repo `Andy0704/team05-fall-detection-band`, pushed `main`, created and pushed feature branches `sensor-sampling`/`detection-rule`/`output-alarm`/`testing`, invited the three teammates as collaborators, and wrote `README.md`.
