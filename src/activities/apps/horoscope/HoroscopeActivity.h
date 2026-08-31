#pragma once

#include "activities/Activity.h"

// "星座运势" (Horoscope): pick a zodiac sign with Up/Down, Confirm rolls a
// fresh daily fortune. Stateless. Sign and fortune data are flash-resident.
class HoroscopeActivity final : public Activity {
 public:
  explicit HoroscopeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Horoscope", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  int signIndex_ = 0;
  const char* rating_ = nullptr;
  const char* overall_ = nullptr;
  void roll();
};