#include "WhatToEatActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <esp_system.h>

#include "components/UITheme.h"
#include "fontIds.h"

namespace {

// Flash-resident dish catalog (string literals live in flash, not DRAM). CJK
// renders via the UI_12 -> CJK_UI_12 fallback in main.cpp.
constexpr const char* kDishes[] = {
    "红烧肉",   "宫保鸡丁", "麻婆豆腐", "糖醋排骨", "鱼香肉丝", "回锅肉",
    "番茄炒蛋", "土豆炖牛腩", "清蒸鲈鱼", "酸菜鱼", "水煮鱼", "火锅",
    "麻辣香锅", "小龙虾", "烤鱼", "卤肉饭", "扬州炒饭", "蛋炒饭",
    "牛肉面", "炸酱面", "兰州拉面", "阳春面", "饺子", "小笼包",
    "生煎包", "煎饼果子", "肉夹馍", "凉皮", "砂锅粥", "皮蛋瘦肉粥",
    "豆浆油条", "肠粉", "虾饺", "烧麦", "螺蛳粉", "酸辣粉",
    "黄焖鸡", "大盘鸡", "剁椒鱼头", "啤酒鸭", "干锅花菜", "地三鲜",
    "醋溜土豆丝", "青椒肉丝", "黑椒牛柳", "蒜蓉西兰花", "咖喱鸡", "黑森林蛋糕",
};
constexpr int kDishCount = static_cast<int>(sizeof(kDishes) / sizeof(kDishes[0]));

// Wheel-spin animation: a short reel of randomly sampled dishes that decelerates
// before settling. Frame count + base period are cheap, stack-only constants.
constexpr int kSpinFrames = 22;
constexpr unsigned long kSpinBaseMs = 40;

}  // namespace

void WhatToEatActivity::onEnter() {
  Activity::onEnter();
  pick();
}

void WhatToEatActivity::pick() {
  if (!spinning_) {
    spinning_ = true;
    spinLeft_ = kSpinFrames;
    spinDish_ = dish_;
  }
  // Restart the reel from wherever we are when re-rolled mid-spin.
  nextFrameMs_ = millis();
  requestUpdate();
}

void WhatToEatActivity::loop() {
  if (spinning_) {
    const unsigned long now = millis();
    if (now >= nextFrameMs_) {
      spinDish_ = kDishes[esp_random() % kDishCount];
      --spinLeft_;
      // Decelerate: frame gap grows as the reel approaches the end.
      const unsigned long period = kSpinBaseMs + (kSpinFrames - spinLeft_) * 18;
      nextFrameMs_ = now + period;
      if (spinLeft_ <= 0) {
        dish_ = spinDish_;
        spinning_ = false;
      }
      requestUpdate();
    }
    // While spinning, ignore re-rolls but still allow leaving.
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      activityManager.goToApps();
    }
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goToApps();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
      mappedInput.wasReleased(MappedInputManager::Button::Up) ||
      mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    pick();
  }
}

void WhatToEatActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_WHAT_TO_EAT_TITLE));

  // Centered card with the dish name in the biggest CJK-capable inline font.
  const int cardW = pageWidth - 2 * metrics.contentSidePadding;
  const int cardH = 200;
  const int marginTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 60;
  const int cx = (pageWidth - cardW) / 2;
  renderer.fillRoundedRect(cx, marginTop, cardW, cardH, metrics.controlRadius, Color::White);
  renderer.drawRoundedRect(cx, marginTop, cardW, cardH, 2, metrics.controlRadius, true);

  const int textY = marginTop + cardH / 2 - renderer.getLineHeight(UI_12_FONT_ID) / 2;
  renderer.drawCenteredText(UI_12_FONT_ID, textY, tr(STR_WHAT_TO_EAT_QUESTION), false,
                            EpdFontFamily::REGULAR);
  const bool rolling = spinning_;
  const char* shown = rolling ? spinDish_ : dish_;
  const int dishY = textY + renderer.getLineHeight(UI_12_FONT_ID) + metrics.verticalSpacing;
  renderer.drawCenteredText(UI_12_FONT_ID, dishY, shown ? shown : "", rolling,
                            EpdFontFamily::BOLD);

  // Reel indicator: a row of three lines that "progresses" while spinning and
  // becomes a full bar once the wheel settles.
  const int barTop = dishY + renderer.getLineHeight(UI_12_FONT_ID) + metrics.verticalSpacing + 14;
  const int barW = 120;
  const int barX = pageWidth / 2 - barW / 2;
  const int segW = (barW - 4) / 3;
  const int filled = rolling ? (kSpinFrames - spinLeft_) % 3 + 1 : 3;
  for (int s = 0; s < 3; ++s) {
    renderer.fillRoundedRect(barX + s * (segW + 2), barTop, segW, 4, 2, (s < filled) ? Color::Black : Color::White);
    renderer.drawRoundedRect(barX + s * (segW + 2), barTop, segW, 4, 1, 2, true);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_WHAT_TO_EAT_REROLL), "", tr(STR_WHAT_TO_EAT_REROLL));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}