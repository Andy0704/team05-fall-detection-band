# CLAUDE.md: Team 05 Fall-Detection Band (BME 3301 Embedded Systems, NCKU)

Guidance for Claude Code and teammates working in this repository.
Design decisions come from the revised proposal (`docs/Team05_Proposal.docx`). Current progress is in `PROJECT_STATUS.md`.

## Project

A waist-worn fall-detection band for older adults who may fall while alone. An ESP32-S3 reads an MPU6050 accelerometer, decides on the board, and raises a local alarm (buzzer + OLED, with a cancel button). There is no wireless alert.

The course has no written exams. Assessment is this group project only:

| Date | Deliverable | Format |
|---|---|---|
| Thu 5 Nov | Proposal | Word + PDF |
| Thu 19 Nov | Progress deliverable | PowerPoint |
| Thu 3 Dec | Demo Day | Word + PowerPoint |

Deliverables and code comments are in English. Team discussion may be in Traditional Chinese.

## Hardware

- ESP32-S3-N16R8 board (16 MB flash, 8 MB octal PSRAM). MPU6050 on I2C at address 0x68, SDA on GPIO 8, SCL on GPIO 9, powered from 3V3 (never 5V).
- MPU6050 INT pin: GPIO4 (wired 2026-11-05).
- OLED, buzzer, push button, power bank and strap: availability and pins are TBD (confirm with a TA). Do not invent pin numbers; add them here once wired.
- If a module does not appear on the I2C bus, ask a TA for a replacement before debugging code.
- Toolchain: PlatformIO, project root at `firmware/`. Board id `esp32-s3-devkitc-1`, framework `arduino`.
- Which physical axis (`ax`/`ay`/`az`) is `a_v` (vertical) depends on the final waist-mounting orientation and is not confirmed yet — see PROJECT_STATUS.md open decision #1. The firmware currently assumes `az` as a placeholder (`firmware/src/fall_detector.cpp`); update it once the band is actually worn.

## The four mandatory specs (hard constraints)

| Spec | Requirement | Does NOT pass |
|---|---|---|
| 01 Sensor and rate | At least one biomedical sensor at 50 Hz or more. Report the **measured** rate (count samples over a known time with `micros()`). | The value of `RATE_HZ` |
| 02 Real time | Read, process and decide on the board while the signal arrives. The laptop may only display or log. | A laptop script reading a saved file; SD card then Python |
| 03 Rule and output | One event rule as numbers with units, each a named constant. Output on the device (LED, OLED, buzzer, vibration). | A bare `if (a > 3.0)`; only `Serial.print` with the laptop plugged in |
| 04 Ten minutes | 10 min continuous, no crash, no dropped samples. Commit the raw log and compute the drop rate from it. | "It ran fine, I watched it" |

## Design decisions (from the proposal)

- **Mounting:** waist (lower back), one sensor axis aligned with the body's vertical axis (`a_v`).
- **Sampling:** 100 Hz target (10 ms period), always reported as measured. A dedicated high-priority FreeRTOS task uses `vTaskDelayUntil()`.
- **Sensor config:** accelerometer ±8 g (4096 LSB/g). DLPF first choice 44 Hz bandwidth (to confirm). Rate, DLPF setting and sensor position are fixed before collecting threshold data and written in every log header.
- **Signals:** SVM = √(ax² + ay² + az²) in g. Trunk tilt θ = arccos(a_v / SVM) in degrees.
- **Rule:** free-fall → impact within a window → trunk tilt above a threshold and SVM within 1 ± `INACTIVITY_SVM_DEV_G` of 1 g for `INACTIVITY_S` (tracked with a running min/max, no variance).
- **Output:** buzzer + OLED "FALL DETECTED". The button cancels. The alarm ends on button press or after `ALARM_TIMEOUT_S`, then returns to monitoring.
- **No blocking:** display and logging run in lower-priority tasks fed by a queue. The OLED refreshes only on state changes and about once per second.
- **I2C contention:** a full OLED frame may take 20–30 ms at 400 kHz (an estimate; measure it with `micros()`), longer than the 10 ms period. Fallbacks: read the MPU6050 FIFO (1024 B, about 170 samples at 100 Hz) or put the OLED on the second I2C controller.
- **Sensor health:** I2C failure or timeout, WHO_AM_I (register 0x75) not 0x68, `SENSOR_IDENTICAL_SAMPLES` identical samples in a row, or no new sample for `SENSOR_STALE_MS`. The device shows "SENSOR ERROR", beeps twice every 5 s, re-initialises every 1 s, and recovers after `SENSOR_RECOVER_SAMPLES` valid samples.

## Named constants (all thresholds live in one config header)

All thresholds live in `firmware/include/config.h`. The fall rule itself is
implemented in `firmware/include/fall_detector.h` /
`firmware/src/fall_detector.cpp`.

Values below are starting points. Only the three marked "handout" come from the course example; the rest are first guesses to be tested.

| Constant | Value | Unit | Status |
|---|---|---|---|
| `SAMPLE_PERIOD_MS` | 10 | ms | design choice |
| `FREEFALL_THRESHOLD_G` | 0.4 | g | handout example |
| `FREEFALL_MIN_MS` | 50 | ms | first guess |
| `IMPACT_THRESHOLD_G` | 2.5 | g | handout example |
| `IMPACT_WINDOW_S` | 0.5 | s | handout example |
| `TILT_THRESHOLD_DEG` | 60 | degrees | first guess |
| `INACTIVITY_S` | 2 | s | first guess |
| `INACTIVITY_SVM_DEV_G` | 0.15 | g | first guess |
| `ALARM_TIMEOUT_S` | 30 | s | first guess |
| `SENSOR_STALE_MS` | 30 | ms | first guess |
| `SENSOR_IDENTICAL_SAMPLES` | 50 | samples | first guess |
| `SENSOR_RECOVER_SAMPLES` | 100 | samples | first guess |

## Code rules

1. Every threshold is a named constant with its unit in the name or a comment, and a comment saying where the number came from (own data file, datasheet page, paper, or "first guess"). No bare numbers in conditions.
2. The sampling task never blocks: no `delay()`, no display calls, no blocking `Serial` writes.
3. The sample counter increments on every scheduled tick, not after a successful read, so a missed tick shows as a gap.
4. `Serial.print` is for logging and debugging only. It is not an output.
5. Log format. Header lines start with `#` and record sampling setting, DLPF setting, accelerometer range, sensor position, firmware commit hash and date. Data lines are `counter,micros,ax,ay,az,state`, with acceleration in g.
6. `micros()` is 32-bit and wraps after about 71.6 min. Use unsigned subtraction when computing intervals.
7. Prefer simple, readable code. Every teammate must be able to explain any part of it on Demo Day.

## Data and honesty rules

- Raw logs go in `data/raw/` and are never edited or re-saved. Derived plots and tables go in `data/processed/`.
- Never report a number that was not measured. Every reported number cites its source file and the script that produced it. If the measured rate differs from `RATE_HZ`, report the measured one.
- Split trials before tuning: about two thirds to tune thresholds, about one third held out and used only to report results.
- Report sensitivity and specificity with trial counts. State limitations: mat landings are softer than a real floor, and participants are healthy team members.
- Safety: falls only onto a mat, with a spotter, never from height.

## Repo layout

```
CLAUDE.md
PROJECT_STATUS.md
README.md          quick-start pointer for the team (hardware, build, branches)
.claude/skills/fall-band-spec-check/SKILL.md
docs/              proposal, slides, final report
firmware/          PlatformIO project (ESP32-S3 sources)
  platformio.ini
  include/         pins.h, config.h, fall_detector.h
  src/             main.cpp, fall_detector.cpp
tools/             log analysis scripts, e.g. tools/analyze_log.py
data/raw/          raw serial logs, committed, never edited
data/processed/    plots and tables derived from raw logs
```

## Working agreements

- Update `PROJECT_STATUS.md` after each work session: what changed, and any measured number with its source file.
- Commit each raw log together with the firmware commit hash it was recorded with.
- Check with the team before changing a rule structure or a design decision above. Threshold values change only with data behind them.
- GitHub (private): https://github.com/Andy0704/team05-fall-detection-band. Branch per work area — `sensor-sampling`, `detection-rule`, `output-alarm`, `testing` — merged into `main` via PR, not pushed to `main` directly. See `README.md` for details.
