// Fall-detection rule (CLAUDE.md "Design decisions"): free-fall -> impact
// within a window -> settle -> lying still (SVM near 1 g for INACTIVITY_MS)
// with trunk tilt above TILT_THRESHOLD_DEG, measured while lying. The output/alarm module
// (firmware/src/main.cpp outputTask) consumes the state this returns and
// calls reset() once the alarm ends (cancel button or ALARM_TIMEOUT_S).
#pragma once

#include <cstdint>

enum class SensorState : uint8_t {
  MONITORING,
  FREE_FALL,
  IMPACT_WAIT,
  SETTLE,
  INACTIVITY_CHECK,
  FALL_DETECTED,
};

const char *sensorStateName(SensorState state);

class FallDetector {
 public:
  // valid: whether this sample's I2C read succeeded. ax_g/ay_g/az_g:
  // acceleration in g (only meaningful when valid). now_us: micros() at
  // this sample. Call every scheduled tick, even when valid is false, so
  // a sensor-read gap mid-sequence can reset the detector instead of
  // letting a stale or partially-observed sequence count as evidence.
  SensorState update(bool valid, float ax_g, float ay_g, float az_g,
                      uint32_t now_us);

  // Called by the output/alarm task once an alarm ends, to return to
  // MONITORING. Only meaningful when the current state is FALL_DETECTED.
  void reset();

 private:
  SensorState state_ = SensorState::MONITORING;
  uint32_t phase_start_us_ = 0;
  uint32_t impact_us_ = 0;  // when the impact was seen (for the timeout)
  float svm_min_g_ = 0.0f;
  float svm_max_g_ = 0.0f;
};
