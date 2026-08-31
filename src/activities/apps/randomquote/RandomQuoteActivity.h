#pragma once

#include <activities/Activity.h>

// "随机台词" (Random line): shows a literary line pulled from a flash classics
// library, rendered with an enlarged, artistic drop-cap. A live extraction of
// the user's currently open book text is intentionally replaced by a curated
// classics shelf for e-ink reliability.
class RandomQuoteActivity final : public Activity {
 public:
  explicit RandomQuoteActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("RandomQuote", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  const char* text_ = nullptr;
  const char* source_ = nullptr;
  void pick();
};