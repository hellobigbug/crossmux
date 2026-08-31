#pragma once

#include "activities/Activity.h"

// "每日一言" (Quote of the day): shows a random quote — a fixed pick for a
// given date so the same day always shows the same line. Confirm rolls to the
// next quote. Stateless.
class QuoteOfDayActivity final : public Activity {
 public:
  explicit QuoteOfDayActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("QuoteOfDay", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  const char* quote_ = nullptr;
  const char* author_ = nullptr;
  void pick();
};