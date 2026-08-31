#include "TamaPetActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <algorithm>

#include "components/UITheme.h"
#include "fontIds.h"

int TamaPetActivity::overalHunger_ = 70;
int TamaPetActivity::overalHappiness_ = 70;
int TamaPetActivity::energy_ = 80;
unsigned long TamaPetActivity::lastTick_ = 0;
unsigned long TamaPetActivity::lastPatMs_ = 0;
bool TamaPetActivity::sleeping_ = false;

namespace {

constexpr int kBarW = 40;
constexpr int kBarH = 6;

}  // namespace

void TamaPetActivity::onEnter() {
  Activity::onEnter();
  lastTick_ = millis();
  requestUpdate();
}

void TamaPetActivity::tick(unsigned long now) {
  const unsigned long elapsed = now - lastTick_;
  if (elapsed < 2000) return;
  lastTick_ = now;
  if (sleeping_) {
    energy_ = std::min(100, energy_ + 3);
    overalHunger_ = std::max(0, overalHunger_ - 1);
  } else {
    overalHunger_ = std::max(0, overalHunger_ - 2);
    energy_ = std::max(0, energy_ - 1);
  }
  if (overalHunger_ == 0 || energy_ == 0) overalHappiness_ = std::max(0, overalHappiness_ - 2);
  requestUpdate();
}

void TamaPetActivity::showBubble(const char* msg) {
  bubbleMsg_ = msg;
  bubbleUntilMs_ = millis() + 1400;
  requestUpdate();
}

void TamaPetActivity::feed() {
  overalHunger_ = std::min(100, overalHunger_ + 20);
  energy_ = std::max(0, energy_ - 3);
  showBubble(tr(STR_TAMA_PET_MSG_FEED));
}

void TamaPetActivity::play() {
  if (sleeping_) return;
  overalHappiness_ = std::min(100, overalHappiness_ + 15);
  energy_ = std::max(0, energy_ - 8);
  showBubble(tr(STR_TAMA_PET_MSG_PLAY));
}

void TamaPetActivity::pat(unsigned long now) {
  // Petting is a gentle, rate-limited mood boost: tap or hold the screen.
  if (now - lastPatMs_ < 300) return;
  lastPatMs_ = now;
  overalHappiness_ = std::min(100, overalHappiness_ + 2);
  showBubble(tr(STR_TAMA_PET_MSG_PAT));
}

void TamaPetActivity::toggleSleep() {
  sleeping_ = !sleeping_;
  requestUpdate();
}

void TamaPetActivity::loop() {
  const unsigned long now = millis();
  tick(now);

  // Bubble auto-expires.
  if (bubbleMsg_ && now >= bubbleUntilMs_) {
    bubbleMsg_ = nullptr;
    requestUpdate(true);
  }

  // Touch to pat the pet (works while the finger stays down, rate-limited).
  int tx = 0;
  int ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty) || mappedInput.isScreenTouchHeld(tx, ty)) {
    pat(now);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goToApps();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    feed();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    play();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    toggleSleep();
  }
}

namespace {

void drawMeterIcon(GfxRenderer& renderer, const int x, const int y, const int value, const int color) {
  renderer.drawRect(x, y, kBarW, kBarH, true);
  const int fillW = (kBarW - 2) * value / 100;
  if (fillW > 0) renderer.fillRect(x + 1, y + 1, fillW, kBarH - 2, true);
  (void)color;
}

}  // namespace

void TamaPetActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_TAMA_PET_TITLE));

  const int cx = pageWidth / 2;
  const int groundY = pageHeight * 3 / 5;

  // Pet body: a rounded blob.
  const int bodyW = 180;
  const int bodyH = 120;
  const int bodyX = cx - bodyW / 2;
  const int bodyY = groundY - bodyH;
  renderer.fillRoundedRect(bodyX, bodyY, bodyW, bodyH, 90, Color::White);
  renderer.drawRoundedRect(bodyX, bodyY, bodyW, bodyH, 2, 90, true);
  // Ears.
  renderer.drawRoundedRect(bodyX - 20, bodyY + 18, 26, 40, 10, 10, true);
  renderer.drawRoundedRect(bodyX + bodyW - 6, bodyY + 18, 26, 40, 10, 10, true);
  // Mood derived from the meters so the face reacts to its own condition.
  enum class Mood { Neutral, Happy, Hungry, Tired };
  Mood mood = Mood::Neutral;
  if (!sleeping_) {
    if (overalHunger_ < 30) {
      mood = Mood::Hungry;
    } else if (energy_ < 25) {
      mood = Mood::Tired;
    } else if (overalHappiness_ >= 70) {
      mood = Mood::Happy;
    }
  }

  // Sleep sweat/tear marker on the forehead when hungry or tired.
  if (mood == Mood::Hungry) {
    renderer.fillRoundedRect(bodyX + 34, bodyY + 2, 8, 12, 4, Color::White);
    renderer.drawRoundedRect(bodyX + 34, bodyY + 2, 8, 12, 2, 4, true);
  }

  // Eyes.
  if (sleeping_) {
    renderer.drawLine(bodyX + bodyW / 2 - 24, bodyY + 46, bodyX + bodyW / 2 - 10, bodyY + 46, 3, true);
    renderer.drawLine(bodyX + bodyW / 2 + 10, bodyY + 46, bodyX + bodyW / 2 + 24, bodyY + 46, 3, true);
  } else if (mood == Mood::Tired) {
    // Droopy (half-closed) eyes.
    renderer.drawLine(bodyX + bodyW / 2 - 24, bodyY + 52, bodyX + bodyW / 2 - 10, bodyY + 44, 3, true);
    renderer.drawLine(bodyX + bodyW / 2 + 10, bodyY + 44, bodyX + bodyW / 2 + 24, bodyY + 52, 3, true);
  } else {
    renderer.fillRect(bodyX + bodyW / 2 - 26, bodyY + 40, 14, 18, true);
    renderer.fillRect(bodyX + bodyW / 2 + 12, bodyY + 40, 14, 18, true);
  }

  // Mouth reacts to mood (smile / sad frown / flat).
  if (!sleeping_) {
    const int mx = cx;
    const int my = bodyY + 68;
    if (mood == Mood::Happy) {
      renderer.drawLine(mx - 16, my, mx, my + 9, 2, true);
      renderer.drawLine(mx, my + 9, mx + 16, my, 2, true);
    } else if (mood == Mood::Hungry) {
      renderer.drawLine(mx - 16, my + 6, mx, my - 4, 2, true);
      renderer.drawLine(mx, my - 4, mx + 16, my + 6, 2, true);
    } else {
      renderer.drawLine(mx - 12, my + 3, mx + 12, my + 3, 2, true);
    }
  }
  // Mouth "z" marks while sleeping.
  if (sleeping_) renderer.drawText(SMALL_FONT_ID, bodyX + bodyW + 8, bodyY, "zzz", false, EpdFontFamily::REGULAR);

  // Action feedback bubble above the head.
  if (bubbleMsg_) {
    const int bh = renderer.getLineHeight(SMALL_FONT_ID) + 8;
    const int bw = renderer.getTextWidth(SMALL_FONT_ID, bubbleMsg_, EpdFontFamily::REGULAR) + 18;
    const int bx = cx - bw / 2;
    const int by = bodyY - bh - 6;
    renderer.fillRoundedRect(bx, by, bw, bh, metrics.controlRadius, Color::White);
    renderer.drawRoundedRect(bx, by, bw, bh, 2, metrics.controlRadius, true);
    renderer.drawLine(cx - 4, by + bh - 2, cx + 4, by + bh - 2, 2, true);
    renderer.drawLine(cx + 4, by + bh - 2, cx, by + bh + 4, 2, true);
    renderer.drawLine(cx, by + bh + 4, cx - 4, by + bh - 2, 2, true);
    renderer.drawText(SMALL_FONT_ID, cx - renderer.getTextWidth(SMALL_FONT_ID, bubbleMsg_, EpdFontFamily::REGULAR) / 2,
                      by + 3, bubbleMsg_, true, EpdFontFamily::REGULAR);
  }

  // Persistent low-state hint under the pet so trouble is unmissable.
  if (!sleeping_ && (mood == Mood::Hungry || mood == Mood::Tired)) {
    const char* hint = (mood == Mood::Hungry) ? tr(STR_TAMA_PET_MSG_HUNGRY) : tr(STR_TAMA_PET_MSG_TIRED);
    renderer.drawCenteredText(SMALL_FONT_ID, groundY + 6, hint, true, EpdFontFamily::BOLD);
  }
  // Feet.
  renderer.fillRoundedRect(bodyX + 14, groundY - 16, 36, 16, 8, Color::White);
  renderer.fillRoundedRect(bodyX + bodyW - 50, groundY - 16, 36, 16, 8, Color::White);
  renderer.drawRoundedRect(bodyX + 14, groundY - 16, 36, 16, 2, 8, true);
  renderer.drawRoundedRect(bodyX + bodyW - 50, groundY - 16, 36, 16, 2, 8, true);

  // Meters on the left.
  const int meterX = metrics.contentSidePadding;
  const int meterY0 = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 10;
  renderer.drawText(SMALL_FONT_ID, meterX, meterY0, tr(STR_TAMA_PET_HUNGER), true, EpdFontFamily::REGULAR);
  drawMeterIcon(renderer, meterX, meterY0 + renderer.getLineHeight(SMALL_FONT_ID) + 4, overalHunger_, 0);
  const int meterY1 = meterY0 + renderer.getLineHeight(SMALL_FONT_ID) + kBarH + 16;
  renderer.drawText(SMALL_FONT_ID, meterX, meterY1, tr(STR_TAMA_PET_HAPPY), true, EpdFontFamily::REGULAR);
  drawMeterIcon(renderer, meterX, meterY1 + renderer.getLineHeight(SMALL_FONT_ID) + 4, overalHappiness_, 0);
  const int meterY2 = meterY1 + renderer.getLineHeight(SMALL_FONT_ID) + kBarH + 16;
  renderer.drawText(SMALL_FONT_ID, meterX, meterY2, tr(STR_TAMA_PET_ENERGY), true, EpdFontFamily::REGULAR);
  drawMeterIcon(renderer, meterX, meterY2 + renderer.getLineHeight(SMALL_FONT_ID) + 4, energy_, 0);

  const auto labels =
      mappedInput.mapLabels(tr(STR_BACK), tr(STR_TAMA_PET_FEED), tr(STR_TAMA_PET_PLAY),
                            sleeping_ ? tr(STR_TAMA_PET_WAKE) : tr(STR_TAMA_PET_SLEEP));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}