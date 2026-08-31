#pragma once

#include "activities/Activity.h"

// "赛博空调" (Cyber AC): a minimal line-art air conditioner with a remote row of
// buttons. Confirm toggles power, Up/Down adjust temperature, Left/Right cycle
// the mode (制冷/制热/送风/除湿). Stateless toy.
class CyberACActivity final : public Activity {
 public:
  explicit CyberACActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("CyberAC", renderer, mappedInput) {}

  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }

 private:
  bool on_ = false;
  int temp_ = 26;
  int mode_ = 0;  // index into kModes
  int phase_ = 0;              // airflow animation phase
  unsigned long animMs_ = 0;   // last airflow frame timestamp
  bool booting_ = false;       // brief power-on reveal animation
  unsigned long bootUntilMs_ = 0;
  void togglePower();
};