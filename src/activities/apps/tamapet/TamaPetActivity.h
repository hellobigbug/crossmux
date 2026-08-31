#pragma once

#include <activities/Activity.h>

// "电子宠物" (Tamagotchi-style pet): a small critter with three meters —
// hunger, happiness, energy. Feed (Confirm), play (Up), and sleep (Down).
// State lives in a static member so the pet persists across app exits within a
// single boot; it decays with real time while the app is open.
class TamaPetActivity final : public Activity {
 public:
  explicit TamaPetActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("TamaPet", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static int overalHunger_, overalHappiness_, energy_;
  static bool sleeping_;
  static unsigned long lastTick_;
  static unsigned long lastPatMs_;
  const char* bubbleMsg_ = nullptr;
  unsigned long bubbleUntilMs_ = 0;
  void tick(unsigned long now);
  void feed();
  void play();
  void toggleSleep();
  void pat(unsigned long now);
  void showBubble(const char* msg);
};