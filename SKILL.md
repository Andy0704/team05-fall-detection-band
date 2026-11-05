---
name: fall-band-spec-check
description: Use when writing, reviewing or measuring firmware, logs or slides for the Team 05 fall-detection band (ESP32-S3 + MPU6050) to check the four BME 3301 specs and compute measured rate, drop rate, sensitivity and specificity.
---

# Fall-band spec check

Checks work on the Team 05 fall-detection band against the four mandatory course specs and computes the numbers the team must report. Design context is in `CLAUDE.md`; current progress is in `PROJECT_STATUS.md`.

## When to use

- Reviewing firmware before a demo or a progress deliverable.
- Analysing a raw serial log (measured rate, drop rate).
- Preparing slides or report text that contain measured numbers.

## 1. Spec checklist for firmware review

For each item, answer pass, fail or unknown, and quote the line of code or file that shows it.

- **01 Rate:** the rate shown on the OLED or log comes from counted samples over time measured with `micros()`, not from a `RATE_HZ` constant. The chosen rate is at least 50 Hz and the reason is written down.
- **02 Real time:** reading, processing and the fall decision all run on the ESP32. The laptop only displays or saves.
- **03 Rule:** every threshold is a named constant with a unit in its name or comment, and a source comment. No bare numeric thresholds inside functions.
- **03 Output:** the buzzer or OLED reacts to the event with no laptop attached. `Serial.print` is not counted.
- **04 Ten minutes:** the firmware has run for at least 600 s without reboot, the raw log is committed, and the drop rate is computed from it.
- **Non-blocking:** the sampling task contains no `delay()`, display call or blocking serial write. The counter increments per scheduled tick.

## 2. Measured sampling rate

- Rate (Hz) = samples counted ÷ elapsed seconds, with elapsed time from `micros()`. Example from the course brief: 6,000 samples in 60.2 s is 99.7 Hz. Report 99.7 Hz, not 100 Hz.
- Use the same convention in every report and say which one it is.
- Also report the spread of the sampling interval (minimum, median, maximum of the difference between consecutive `micros()` values).

## 3. Drop rate from a ten-minute log

Log format: `#` header lines, then `counter,micros,ax,ay,az,state`.

1. Parse data lines only. Skip `#` lines. Count malformed lines separately.
2. Expected samples = last counter − first counter + 1. Received = number of distinct counter values present.
3. Missing = expected − received. Drop rate (%) = missing ÷ expected × 100.
   Course example: 59,940 received out of 60,000 gives 60 ÷ 60,000 = 0.1 %.
4. Check the counter is increasing. A counter that goes back to a low value means a reboot, which fails "no crash". Report where it happened.
5. Check the duration. (last `micros` − first `micros`) must be at least 600 s. `micros()` is 32-bit and wraps at about 4,295 s, so use unsigned subtraction.
6. Flag late samples: an interval above 15 ms (1.5 × the 10 ms period). Report these separately from missing counters, because a late sample is jitter and a missing counter is a lost sample.
7. A gap in the counter can be a lost tick on the board or a line lost on the serial link. If the firmware keeps its own count of overruns, compare the two.

Report: expected, received, missing, drop rate (%), duration (s), measured rate (Hz), late-sample count, reboot count, and the log file name.

## 4. Threshold provenance

Every threshold in the code and in the proposal needs one source: own measurement (name the file), datasheet (page number), paper, or first guess with the test that will check it. Update the threshold table in `PROJECT_STATUS.md` whenever a value changes, and never change a value without data behind it.

## 5. Detection performance

- Split trials before tuning: about two thirds for tuning, about one third held out for reporting.
- Sensitivity = detected falls ÷ all falls. Specificity = non-fall trials without an alarm ÷ all non-fall trials. Always give the counts, for example "9/10 falls detected, 2 false alarms in 20 sit-downs", and also list false alarms per activity (sit-down, lying down, jumping, walking, stairs).
- State limitations: mat landings are softer than a real floor, so the impact peak is probably underestimated; participants are healthy team members.

## 6. Evidence for the 19 November slides

| Spec | Show on a slide |
|---|---|
| 01 Rate | Screenshot of the measured rate in Hz from a real run |
| 02 Real time | The decision happening live, with no script reading a saved file |
| 03 Rule | One line of code with the named constant and its unit |
| 03 Output | Buzzer or OLED reacting when the event happens |
| 04 Ten min | Link to the committed raw log and the drop rate worked out from it |

It is better to show four honest, measured numbers than a device that does a lot with nothing measured.

## Output format

Reply with a short table: Spec, Evidence found, Pass / Fail / Unknown, Gap to close. Then list the numbers computed, each with its source file. Never fill a number that was not measured; write "not measured yet" instead.
