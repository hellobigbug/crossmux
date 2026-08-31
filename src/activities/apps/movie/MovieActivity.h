#pragma once

#include <activities/Activity.h>

// "电影推荐" (Film picker): draws a curated film card — title, year, genre and a
// short synopsis — with a stylized poster frame. Confirm rolls to the next
// pick. The catalog is flash-resident and offline-first for e-ink reliability;
// a live network + poster download is intentionally out of scope.
class MovieActivity final : public Activity {
 public:
  explicit MovieActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Movie", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  int index_ = 0;
  void pick();
};