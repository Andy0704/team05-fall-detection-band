#include "fall_detector.h"

#include <math.h>

#include "config.h"

namespace {

constexpr float RAD_TO_DEG = 57.29577951308232f;

// a_v (the body's vertical axis component) is -ax for the current waist
// mounting. Source: data/raw/2026-11-17_onbody-pilot/
// 2026-11-17_standing-still_axis-check.log (wearer standing still, 14.76 s):
// mean ax = -0.936 g, ay = 0.044 g, az = 0.258 g, so gravity lies along -ax
// (15.6 deg off). Negated so that upright standing gives tilt near 0 deg.
// Re-check this if the band's mounting orientation changes.
float verticalComponentG(float ax_g, float ay_g, float az_g) {
  (void)ay_g;
  (void)az_g;
  return -ax_g;
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
    case SensorState::SETTLE:
      return "SETTLE";
    case SensorState::INACTIVITY_CHECK:
      return "INACTIVITY_CHECK";
    case SensorState::FALL_DETECTED:
      return "FALL_DETECTED";
  }
  return "UNKNOWN";
}

SensorState FallDetector::update(bool valid, float ax_g, float ay_g,
                                   float az_g, uint32_t now_us) {
  if (!valid) {
    // CLAUDE.md anticipates I2C read failures as a normal condition to
    // handle. A gap mid-sequence means we cannot trust whatever happened
    // during it, so drop any in-progress detection back to MONITORING
    // rather than let a stale or partially-observed sequence (e.g. an
    // impact or an inactivity window) count as evidence once reads
    // resume.
    switch (state_) {
      case SensorState::FREE_FALL:
      case SensorState::IMPACT_WAIT:
      case SensorState::SETTLE:
      case SensorState::INACTIVITY_CHECK:
        state_ = SensorState::MONITORING;
        break;
      case SensorState::MONITORING:
      case SensorState::FALL_DETECTED:
        break;
    }
    return state_;
  }

  const float svm = sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);

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
      // Check the window expiry first: an impact that arrives after the
      // window has closed must not be accepted (checking svm first would
      // accept a stale impact regardless of how late it is).
      const uint32_t elapsedMs = (now_us - phase_start_us_) / 1000;
      if (elapsedMs > IMPACT_WINDOW_MS) {
        state_ = SensorState::MONITORING; // no impact within the window
      } else if (svm > IMPACT_THRESHOLD_G) {
        state_ = SensorState::SETTLE;
        phase_start_us_ = now_us;
        impact_us_ = now_us;
      }
      break;
    }

    case SensorState::SETTLE: {
      // Ignore bounces and rolling right after the impact. Seeding the
      // still check with the impact sample (> IMPACT_THRESHOLD_G) would
      // make it fail on the very next sample.
      const uint32_t elapsedMs = (now_us - phase_start_us_) / 1000;
      if (elapsedMs >= POST_IMPACT_SETTLE_MS) {
        state_ = SensorState::INACTIVITY_CHECK;
        phase_start_us_ = now_us;  // start of the current still window
        svm_min_g_ = svm;
        svm_max_g_ = svm;
      }
      break;
    }

    case SensorState::INACTIVITY_CHECK: {
      if (svm < svm_min_g_) svm_min_g_ = svm;
      if (svm > svm_max_g_) svm_max_g_ = svm;

      const bool moved =
          (svm_max_g_ - STANDING_SVM_G > INACTIVITY_SVM_DEV_G) ||
          (STANDING_SVM_G - svm_min_g_ > INACTIVITY_SVM_DEV_G);
      if (moved) {
        // Restart the still window from this sample rather than abort,
        // so e.g. rolling over once after landing still counts.
        phase_start_us_ = now_us;
        svm_min_g_ = svm;
        svm_max_g_ = svm;
      } else if ((now_us - phase_start_us_) / 1000 >= INACTIVITY_MS) {
        // Still for INACTIVITY_MS: now check the lying posture. Here svm
        // is within STANDING_SVM_G +/- INACTIVITY_SVM_DEV_G (> 0), so the
        // division is safe and the reading is mostly gravity.
        const float av = verticalComponentG(ax_g, ay_g, az_g);
        float ratio = av / svm;
        if (ratio > 1.0f) ratio = 1.0f;
        if (ratio < -1.0f) ratio = -1.0f;
        const float tiltDeg = acosf(ratio) * RAD_TO_DEG;
        state_ = (tiltDeg > TILT_THRESHOLD_DEG)
                     ? SensorState::FALL_DETECTED
                     : SensorState::MONITORING;  // still but upright
        break;
      }

      if ((now_us - impact_us_) / 1000 > INACTIVITY_TIMEOUT_MS) {
        state_ = SensorState::MONITORING;  // never lay still: not a fall
      }
      break;
    }

    case SensorState::FALL_DETECTED:
      // Held here until the output/alarm task calls reset().
      break;
  }

  return state_;
}

void FallDetector::reset() { state_ = SensorState::MONITORING; }
