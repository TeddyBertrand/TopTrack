#include "client/game/car_controller.hpp"

#include <raylib.h>

namespace toptrack::client::game {

void CarController::update(float dtSeconds) {
  toptrack::CarInput input;
  if (IsKeyDown(KEY_UP)) input.throttle = 1.0f;
  if (IsKeyDown(KEY_DOWN)) input.throttle = -1.0f;
  if (IsKeyDown(KEY_LEFT)) input.steer = -1.0f;
  if (IsKeyDown(KEY_RIGHT)) input.steer = 1.0f;
  input.handbrake = IsKeyDown(KEY_SPACE);

  state_ = toptrack::stepCar(state_, input, tuning_, dtSeconds);
  elapsedSeconds_ += dtSeconds;

  toptrack::protocol::GhostFrame frame;
  frame.t = elapsedSeconds_;
  frame.x = state_.x;
  frame.y = state_.y;
  frame.headingRad = state_.headingRad;
  ghost_.push_back(frame);
}

void CarController::resetRun() {
  state_ = toptrack::CarState{};
  ghost_.clear();
  elapsedSeconds_ = 0.0f;
}

} // namespace toptrack::client::game
