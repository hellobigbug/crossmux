#pragma once

#include "activities/Activity.h"

// "今天吃什么" (What to eat today): a tiny single-screen generator that picks a
// random dish from a static flash table. Stateless — recreates on every entry
// (UglyAvatar pattern, no Store/menu Activity needed).
class WhatToEatActivity final : public Activity {
 public:
  explicit WhatToEatActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("WhatToEat", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  const char* dish_ = nullptr;
  void pick();
};