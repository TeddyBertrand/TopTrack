#include "toptrack/physics.hpp"

#include <algorithm>
#include <cmath>

namespace toptrack {

CarState stepCar(const CarState &state, const CarInput &input,
                  const CarTuning &tuning, float dtSeconds) {
  CarState next = state;

  // Gear selection: pick highest gear whose max speed still exceeds
  // current speed, giving a stepped acceleration curve instead of a
  // single flat top speed.
  int gear = 1;
  for (int g = 1; g <= 5; ++g) {
    if (state.speed >= tuning.maxSpeed[g] * 0.9f) gear = std::min(g + 1, 5);
  }
  next.gear = gear;

  float targetTopSpeed = tuning.maxSpeed[gear];
  if (input.throttle > 0.0f) {
    next.speed += tuning.acceleration * input.throttle * dtSeconds;
    next.speed = std::min(next.speed, targetTopSpeed);
  } else if (input.throttle < 0.0f) {
    next.speed += tuning.braking * input.throttle * dtSeconds;
  }
  next.speed = std::max(next.speed, -tuning.maxSpeed[1] * 0.5f);

  float effectiveGrip =
      std::max(0.1f, 1.0f - tuning.gripDecayAtSpeed * std::fabs(next.speed));
  if (input.handbrake) effectiveGrip *= tuning.driftGripFactor;

  float desiredSlip = input.handbrake ? input.steer * 0.6f : 0.0f;
  next.slipAngleRad +=
      (desiredSlip - next.slipAngleRad) * std::min(1.0f, tuning.slipRecoveryRate * dtSeconds);

  float turn = input.steer * tuning.turnRateRad * effectiveGrip * dtSeconds;
  next.headingRad += turn * (next.speed >= 0 ? 1.0f : -1.0f);

  float travelAngle = next.headingRad + next.slipAngleRad;
  next.x += std::cos(travelAngle) * next.speed * dtSeconds;
  next.y += std::sin(travelAngle) * next.speed * dtSeconds;

  return next;
}

} // namespace toptrack
