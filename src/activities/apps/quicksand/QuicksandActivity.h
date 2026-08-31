#pragma once

#include <activities/Activity.h>

// "流沙效果" (Quicksand): a slow, drifting ripple field. With no input the
// ripples flow gently (time-driven). Any tap (or, on button-only boards, any
// of the four directional buttons) drops a ripple that pushes back against the
// flow and expands outward. Stateless; throttled for e-ink refresh cost.
class QuicksandActivity final : public Activity {
 public:
  explicit QuicksandActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Quicksand", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }

 private:
  unsigned long lastFrameMs_ = 0;
  unsigned long startMs_ = 0;
  float driverX_ = 0.5f;
  float driverY_ = 0.5f;
  float driverVx_ = 0.0003f;
  float driverVy_ = 0.0002f;
  // Up to a few active ripple centers (normalized 0..1) with their age.
  float rippleXs_[4] = {-1.f, -1.f, -1.f, -1.f};
  float rippleYs_[4] = {-1.f, -1.f, -1.f, -1.f};
  int rippleN_ = 0;
  void addRippleAt(float nx, float ny);
};