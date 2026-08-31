#include "QuoteOfDayActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <esp_system.h>
#include <time.h>

#include "components/UITheme.h"
#include "fontIds.h"
#include "util/TimeUtils.h"

namespace {

struct Quote {
  const char* text;
  const char* author;
};

constexpr Quote kQuotes[] = {
    {"读万卷书，行万里路。", "刘彝"},
    {"书籍是全世界的营养品。", "莎士比亚"},
    {"生活不是为了赶路，而是为了感受路。", "佚名"},
    {"世界上只有一种真正的英雄主义，那就是在认清生活的真相后依然热爱生活。", "罗曼·罗兰"},
    {"路漫漫其修远兮，吾将上下而求索。", "屈原"},
    {"黑夜给了我黑色的眼睛，我却用它寻找光明。", "顾城"},
    {"种一棵树最好的时间是十年前，其次是现在。", "佚名"},
    {"你现在的努力，是为了让未来的自己感谢现在的自己。", "佚名"},
    {"人生的道路虽然漫长，但紧要处常常只有几步。", "柳青"},
    {"凡是过往，皆为序章。", "莎士比亚"},
    {"不必太在意别人的目光，因为真正的观众只有你自己。", "佚名"},
    {"你无法改变风向，但可以调整航向。", "佚名"},
    {"愿你眼里有光，心中有海。", "佚名"},
    {"与其抱怨黑暗，不如点亮蜡烛。", "佚名"},
    {"脚步丈量不到的，阅读可以。", "佚名"},
};
constexpr int kQuoteCount = static_cast<int>(sizeof(kQuotes) / sizeof(kQuotes[0]));

}  // namespace

void QuoteOfDayActivity::onEnter() {
  Activity::onEnter();
  pick();
}

void QuoteOfDayActivity::pick() {
  quote_ = kQuotes[esp_random() % kQuoteCount].text;
  author_ = kQuotes[esp_random() % kQuoteCount].author;
  requestUpdate();
}

void QuoteOfDayActivity::loop() {
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

void QuoteOfDayActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_QUOTE_OF_DAY_TITLE));

  const int contentX = metrics.contentSidePadding;
  const int contentY = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 40;
  const int contentW = pageWidth - 2 * contentX;
  const int contentH = pageHeight - contentY - metrics.buttonHintsHeight - metrics.verticalSpacing - 40;

  // Large opening quote glyph.
  renderer.drawText(UI_12_FONT_ID, contentX, contentY, "“", true, EpdFontFamily::BOLD);

  // Wrap the quote body (CJK renders via UI_12 -> CJK_UI_12 fallback).
  const auto lines = renderer.wrappedText(UI_12_FONT_ID, quote_, contentW, 5, EpdFontFamily::REGULAR);
  int y = contentY + renderer.getLineHeight(UI_12_FONT_ID) * 2;
  for (const auto& line : lines) {
    y += renderer.getLineHeight(UI_12_FONT_ID);
    renderer.drawText(UI_12_FONT_ID, contentX, y, line.c_str(), true, EpdFontFamily::REGULAR);
  }
  if (!lines.empty()) y += metrics.verticalSpacing;
  if (author_) renderer.drawText(SMALL_FONT_ID, contentX, y, author_, true, EpdFontFamily::BOLD);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_QUOTE_OF_DAY_NEXT), "", tr(STR_QUOTE_OF_DAY_NEXT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}