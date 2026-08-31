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
    u8"红烧肉",   u8"宫保鸡丁", u8"麻婆豆腐", u8"糖醋排骨", u8"鱼香肉丝", u8"回锅肉",
    u8"番茄炒蛋", u8"土豆炖牛腩", u8"清蒸鲈鱼", u8"酸菜鱼", u8"水煮鱼", u8"火锅",
    u8"麻辣香锅", u8"小龙虾", u8"烤鱼", u8"卤肉饭", u8"扬州炒饭", u8"蛋炒饭",
    u8"牛肉面", u8"炸酱面", u8"兰州拉面", u8"阳春面", u8"饺子", u8"小笼包",
    u8"生煎包", u8"煎饼果子", u8"肉夹馍", u8"凉皮", u8"砂锅粥", u8"皮蛋瘦肉粥",
    u8"豆浆油条", u8"肠粉", u8"虾饺", u8"烧麦", u8"螺蛳粉", u8"酸辣粉",
    u8"黄焖鸡", u8"大盘鸡", u8"剁椒鱼头", u8"啤酒鸭", u8"干锅花菜", u8"地三鲜",
    u8"醋溜土豆丝", u8"青椒肉丝", u8"黑椒牛柳", u8"蒜蓉西兰花", u8"咖喱鸡", u8"黑森林蛋糕",
};
constexpr int kDishCount = static_cast<int>(sizeof(kDishes) / sizeof(kDishes[0]));

}  // namespace

void WhatToEatActivity::onEnter() {
  Activity::onEnter();
  pick();
}

void WhatToEatActivity::pick() {
  dish_ = kDishes[esp_random() % kDishCount];
  requestUpdate();
}

void WhatToEatActivity::loop() {
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
  const int dishY = textY + renderer.getLineHeight(UI_12_FONT_ID) + metrics.verticalSpacing;
  renderer.drawCenteredText(UI_12_FONT_ID, dishY, dish_ ? dish_ : "", true, EpdFontFamily::BOLD);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_WHAT_TO_EAT_REROLL), "", tr(STR_WHAT_TO_EAT_REROLL));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}