# Team 05 Fall-Detection Band

BME 3301 Embedded Systems, NCKU. A waist-worn fall-detection band for older
adults who may fall while alone: an ESP32-S3 reads an MPU6050 accelerometer,
decides on the board, and raises a local alarm (buzzer + OLED, with a cancel
button). There is no wireless alert.

Full project guidance (hardware, design decisions, named constants, code
rules, data/honesty rules) lives in [`CLAUDE.md`](CLAUDE.md). Current
progress and measured numbers live in
[`PROJECT_STATUS.md`](PROJECT_STATUS.md) — read that before relying on any
number in this README, since it is updated far more often.

## Hardware

- ESP32-S3-N16R8 (16 MB flash, 8 MB octal PSRAM).
- MPU6050 on I2C: SDA → GPIO8, SCL → GPIO9, INT → GPIO4, VCC → 3V3, GND →
  GND.
- OLED, buzzer, push button, power bank, strap: pins TBD, confirm with a TA
  before wiring.

## Toolchain

PlatformIO. Project root is `firmware/`.

```
cd firmware
pio run                                    # build
pio run -t upload --upload-port <port>     # flash
pio device monitor -p <port> -b 115200     # view Serial output (debug only)
```

## Repo layout

```
CLAUDE.md          guidance for this project (read this first)
PROJECT_STATUS.md  current progress, measured numbers, open decisions
firmware/          PlatformIO project (ESP32-S3 sources)
tools/             log analysis scripts, e.g. tools/analyze_log.py
data/raw/          raw serial logs, committed, never edited or re-saved
data/processed/    plots and tables derived from raw logs
docs/              proposal, slides, final report
```

## Branch workflow

`main` plus one feature branch per work area:

- `sensor-sampling` — IMU sampling task, logging
- `detection-rule` — free-fall / impact / tilt / inactivity rule
- `output-alarm` — buzzer, OLED, cancel button
- `testing` — trials, log analysis, sensitivity/specificity reporting

Work on your branch, open a PR into `main` to integrate. Resolve conflicts
in the PR rather than pushing straight to `main`. Everyone should still be
able to explain any part of the merged code on Demo Day (CLAUDE.md code
rule 7).

## Code rules (summary)

See `CLAUDE.md` for the full list. The two that matter most when merging:

1. Every threshold is a named constant with its unit and a comment on
   where the number came from. No bare numbers in conditions.
2. The sampling task never blocks: no `delay()`, no display calls, no
   blocking `Serial` writes.
