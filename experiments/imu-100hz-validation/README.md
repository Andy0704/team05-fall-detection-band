# MPU6050 100 Hz hardware validation

Written by Joaquin ([#1](https://github.com/Andy0704/team05-fall-detection-band/pull/1)).
Originally uploaded to the repo root via GitHub's web upload; moved here to
`experiments/imu-100hz-validation/` (as the original PR description intended)
so it does not collide with the team's `README.md` and `firmware/`. Content
below is otherwise unchanged.

**Project:** Group 05 — Fall-Detection Band  
**Test date:** 2026-11-05  
**Hardware:** ESP32-S3 DevKitC-1 + MPU6050 (I2C address `0x68`)  
**Wiring:** SDA GPIO 8, SCL GPIO 9, VCC 3.3 V, GND GND  
**Environment:** VS Code + PlatformIO, Arduino framework

## Included programs

- `01_I2C_Scanner.cpp` — scans I2C addresses and reads `WHO_AM_I`; returned `0x68`.
- `02_Acceleration_Readings.cpp` — reads `ax`, `ay`, `az` in g and computes signal vector magnitude (SVM).
- `03_100Hz_Sampling.cpp` — schedules reads at 10,000 microsecond intervals, logs a sample counter/timestamp/accelerations/SVM and reports measured rate.
- `platformio.ini` — PlatformIO configuration for reproducing the tests.
- The original serial monitor capture (board startup messages and rate summaries) is kept byte-exact in
  [`data/raw/2026-11-05_joaquin-standalone-validation_10min_100Hz.log`](../../data/raw/2026-11-05_joaquin-standalone-validation_10min_100Hz.log),
  per CLAUDE.md's "raw logs go in `data/raw/`" rule. Independently re-verified with `tools/analyze_log.py`;
  see [`data/processed/2026-11-05_joaquin-standalone-validation_10min_100Hz_rate.txt`](../../data/processed/2026-11-05_joaquin-standalone-validation_10min_100Hz_rate.txt).

## Measured results from the uploaded log

| Metric | Result |
| --- | --- |
| Valid sample records | 61,164 |
| Counter range | 1 to 61,164 |
| Elapsed time between first and last sample | 611.63 s |
| Average sampling rate from timestamps | 100.000001 Hz |
| Timestamp interval (minimum / maximum) | 9,993 / 10,001 us |
| Missing sample counter values | 0 |
| Five-second summary lines | 122 |
| Summary lines reporting missed ticks | 0 |
| Mean stationary SVM (uncalibrated) | 0.8892 g |

## Important limitations

- This is a **sampling prototype**, not the completed fall-detection device.
- Accelerometer calibration is pending; stationary SVM is below the expected 1 g.
- The firmware uses a single-loop `micros()` schedule. The proposed FreeRTOS sampling/processing/logging architecture is **not yet implemented**.
- No physical buzzer, OLED, or button was available for this test; serial output is **not** the final on-device alert.
- A complete log with contiguous counters and no reported missed ticks supports the recorded-run performance, but does not prove that future firmware revisions will have the same behavior.
- The three `.cpp` files are independent historical examples. **Do not put all of them together under `src/`**: each defines `setup()` and `loop()`. For testing, copy only the desired program to `src/main.cpp`.
- The raw log's own header (`# MPU6050 100 Hz sampling test`) does not record the DLPF setting, accelerometer range, sensor position, firmware commit hash, or date that CLAUDE.md's logging rule (code rule 5) asks every log header to carry — those facts are only available from this README and the git history around PR #1, not from the log file itself. The raw log is never edited to backfill this (CLAUDE.md: raw logs are never edited or re-saved), so treat it as supporting evidence rather than a log that is fully self-describing on its own.

## Next steps

1. Perform a six-position accelerometer calibration with the device held still.
2. Implement on-board fall-state detection; validate thresholds against labeled motion trials.
3. Add FreeRTOS separation of sampling and output/logging; repeat the ten-minute test under full load.
4. Add physical alarm output and false-alarm cancellation once components are available.
