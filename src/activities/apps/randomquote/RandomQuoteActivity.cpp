#include "RandomQuoteActivity.h"

#include <Arduino.h>
#include <Utf8.h>
#include <string>
#include <esp_system.h>

#include "components/UITheme.h"
#include "fontIds.h"

namespace {

struct Line {
  const char* text;
  const char* source;
};

constexpr Line kLines[] = {
    {u8"生活不可能像你想象的那么好，但也不会像你想象的那么糟。", u8"《童年》"},
    {u8"世界上只有一种真正的英雄主义，那就是在认清生活真相后依然热爱生活。", u8"《名人传》"},
    {u8"满纸荒唐言，一把辛酸泪。都云作者痴，谁解其中味？", u8"《红楼梦》"},
    {u8"你把我养大，就是要让我学会，把思念埋得比海还深。", u8"《边城》"},
    {u8"我们一路奋战，不是为了改变世界，而是为了不让世界改变我们。", u8"《熔炉》"},
    {u8"人要有翻篇的勇气，也要有等待的耐心。", u8"《追风筝的人》"},
    {u8"凡事先问自己，是否值得；再问天地，是否宽厚。", u8"《边城》"},
    {u8"生活不止眼前的苟且，还有诗和远方的田野。", u8"《生活不止眼前的苟且》"},
    {u8"一个人可以被毁灭，但不能被打败。", u8"《老人与海》"},
    {u8"所有的相遇都是久别重逢。", u8"《一代宗师》"},
    {u8"那一瞬间，所有的灯都亮了起来。", u8"《百年孤独》"},
    {u8"往前走，别回头，所有的美好都在路上。", u8"《活着》"},
};
constexpr int kLineCount = static_cast<int>(sizeof(kLines) / sizeof(kLines[0]));

}  // namespace

void RandomQuoteActivity::onEnter() {
  Activity::onEnter();
  pick();
}

void RandomQuoteActivity::pick() {
  const Line& l = kLines[esp_random() % kLineCount];
  text_ = l.text;
  source_ = l.source;
  requestUpdate();
}

void RandomQuoteActivity::loop() {
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

void RandomQuoteActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_RANDOM_QUOTE_TITLE));

  const int margin = metrics.contentSidePadding;
  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 30;
  const int contentW = pageWidth - 2 * margin;

  // Split off the first UTF-8 codepoint for the enlarged artistic drop cap.
  const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text_);
  const uint32_t firstCp = utf8NextCodepoint(&cursor);
  const char* restStart = reinterpret_cast<const char*>(cursor);
  std::string firstChar;
  utf8AppendCodepoint(firstCp, firstChar);

  // Drop cap: first character in the largest CJK-capable UI font (UI_12, whose
  // CJK fallback is registered in main.cpp).
  const int dropCapY = contentTop;
  renderer.drawText(UI_12_FONT_ID, margin, dropCapY, firstChar.c_str(), true, EpdFontFamily::BOLD);

  // Body wraps beside / below the drop cap in the smaller body font.
  const int dropCapW = renderer.getLineHeight(UI_12_FONT_ID) + 8;
  const auto bodyLines = renderer.wrappedText(SMALL_FONT_ID, restStart, contentW - dropCapW, 6, EpdFontFamily::REGULAR);
  int y = dropCapY + renderer.getLineHeight(UI_12_FONT_ID);
  for (int i = 0; static_cast<size_t>(i) < bodyLines.size(); ++i) {
    const int x = (i == 0) ? margin + dropCapW : margin;
    renderer.drawText(SMALL_FONT_ID, x, y, bodyLines[i].c_str(), true, EpdFontFamily::REGULAR);
    y += renderer.getLineHeight(SMALL_FONT_ID);
  }

  if (source_) renderer.drawText(SMALL_FONT_ID, margin, y + metrics.verticalSpacing, source_, true, EpdFontFamily::BOLD);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_RANDOM_QUOTE_NEXT), "", tr(STR_RANDOM_QUOTE_NEXT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}