// Fall-detection rule: free-fall -> impact within a window -> trunk tilt
// + inactivity (CLAUDE.md "Design decisions"). No on-device alarm output
// yet; that is a separate module (see the output-alarm branch), which
// will consume the state this returns and will need its own way to
// clear FALL_DETECTED (cancel button or ALARM_TIMEOUT_S).
#pragma once

#include <cstdint>

enum class SensorState : uint8_t {
  MONITORING,
  FREE_FALL,
  IMPACT_WAIT,
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

 private:
  SensorState state_ = SensorState::MONITORING;
  uint32_t phase_start_us_ = 0;
  float svm_min_g_ = 0.0f;
  float svm_max_g_ = 0.0f;
};
