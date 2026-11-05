#include "fall_detector.h"

#include <math.h>

#include "config.h"

namespace {

constexpr float RAD_TO_DEG = 57.29577951308232f;

// PLACEHOLDER: a_v (the body's vertical axis component) is assumed to be
// az until the team confirms the physical mounting orientation on the
// waist (PROJECT_STATUS.md open decision #1). Update this if a different
// axis turns out to be vertical once the band is actually worn.
float verticalComponentG(float ax_g, float ay_g, float az_g) {
  (void)ax_g;
  (void)ay_g;
  return az_g;
}

}  // namespace

const char *sensorStateName(SensorState state) {
  switch (state) {
    case SensorState::MONITORING:
      return "MONITORING";
    case SensorState::FREE_FALL:
      return "FREE_FALL";
    case SensorState::IMPACT_WAIT:
      return "IMPACT_WAIT";
    case SensorState::INACTIVITY_CHECK:
      return "INACTIVITY_CHECK";
    case SensorState::FALL_DETECTED:
      return "FALL_DETECTED";
  }
  return "UNKNOWN";
}

SensorState FallDetector::update(float ax_g, float ay_g, float az_g,
                                   uint32_t now_us) {
  const float svm = sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
  const float av = verticalComponentG(ax_g, ay_g, az_g);

  float ratio = av / svm;
  if (ratio > 1.0f) ratio = 1.0f;
  if (ratio < -1.0f) ratio = -1.0f;
  const float tiltDeg = acosf(ratio) * RAD_TO_DEG;

  switch (state_) {
    case SensorState::MONITORING:
      if (svm < FREEFALL_THRESHOLD_G) {
        state_ = SensorState::FREE_FALL;
        phase_start_us_ = now_us;
      }
      break;

    case SensorState::FREE_FALL:
      if (svm < FREEFALL_THRESHOLD_G) {
        const uint32_t elapsedMs = (now_us - phase_start_us_) / 1000;
        if (elapsedMs >= FREEFALL_MIN_MS) {
          state_ = SensorState::IMPACT_WAIT;
          phase_start_us_ = now_us;
        }
      } else {
        state_ = SensorState::MONITORING; // too short to be a real free-fall
      }
      break;

    case SensorState::IMPACT_WAIT: {
      if (svm > IMPACT_THRESHOLD_G) {
        if (tiltDeg > TILT_THRESHOLD_DEG) {
          state_ = SensorState::INACTIVITY_CHECK;
          phase_start_us_ = now_us;
          svm_min_g_ = svm;
          svm_max_g_ = svm;
        } else {
          state_ = SensorState::MONITORING; // impact without a fall-like tilt
        }
      } else {
        const uint32_t elapsedMs = (now_us - phase_start_us_) / 1000;
        if (elapsedMs > IMPACT_WINDOW_MS) {
          state_ = SensorState::MONITORING; // no impact within the window
        }
      }
      break;
    }

    case SensorState::INACTIVITY_CHECK: {
      if (svm < svm_min_g_) svm_min_g_ = svm;
      if (svm > svm_max_g_) svm_max_g_ = svm;

      const bool stillMoving = (svm_max_g_ - 1.0f > INACTIVITY_SVM_DEV_G) ||
                                (1.0f - svm_min_g_ > INACTIVITY_SVM_DEV_G);
      if (stillMoving) {
        state_ = SensorState::MONITORING;
        break;
      }

      const uint32_t elapsedMs = (now_us - phase_start_us_) / 1000;
      if (elapsedMs >= INACTIVITY_MS) {
        state_ = SensorState::FALL_DETECTED;
      }
      break;
    }

    case SensorState::FALL_DETECTED:
      // Held here until the output/alarm module resets it. Not
      // implemented yet - see the output-alarm branch.
      break;
  }

  return state_;
}
