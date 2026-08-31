#pragma once

#include "activities/Activity.h"

// "答案之书" (Book of Answers): a single-screen generator that returns a
// short, oracular answer at random. Stateless. Confirm re-asks; the answer
// location is a touch of retro mystique.
class BookAnswersActivity final : public Activity {
 public:
  explicit BookAnswersActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("BookAnswers", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  const char* answer_ = nullptr;
  int page_ = 1;
  bool revealing_ = false;      // page-turn reveal in progress
  unsigned long revealUntilMs_ = 0;
  unsigned long animMs_ = 0;
  int animPage_ = 0;
  void ask();
};