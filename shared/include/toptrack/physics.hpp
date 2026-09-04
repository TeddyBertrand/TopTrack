#pragma once

namespace toptrack {

// Bespoke top-down arcade car model: gears give a stepped power curve,
// drift is modeled as a deliberate grip/slip split rather than a full
// tire-friction simulation, matched to Trackmania's 2D-projected feel.
struct CarInput {
  float throttle = 0.0f; // -1 (brake/reverse) .. 1 (accelerate)
  float steer = 0.0f;    // -1 (left) .. 1 (right)
  bool handbrake = false;
};

struct CarState {
  float x = 0.0f;
  float y = 0.0f;
  float headingRad = 0.0f;
  float speed = 0.0f;      // forward speed, m/s
  float slipAngleRad = 0.0f; // drift angle vs heading
  int gear = 1;
};

struct CarTuning {
  float maxSpeed[6] = {0, 12, 22, 32, 42, 52}; // index 0 unused, gears 1-5
  float acceleration = 18.0f;
  float braking = 26.0f;
  float turnRateRad = 2.4f;
  float gripDecayAtSpeed = 0.015f; // higher speed -> lower effective grip
  float driftGripFactor = 0.35f;  // grip multiplier while handbrake held
  float slipRecoveryRate = 4.0f;
};

// Advances CarState by dtSeconds given input and tuning. Pure function so
// client (prediction) and server (validation/re-simulation) share exact
// behavior with no drift between implementations.
CarState stepCar(const CarState &state, const CarInput &input,
                  const CarTuning &tuning, float dtSeconds);

} // namespace toptrack
