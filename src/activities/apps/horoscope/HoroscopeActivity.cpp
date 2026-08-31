#include "HoroscopeActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <esp_system.h>

#include "activities/apps/GameUi.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {

constexpr const char* kSigns[] = {
    u8"白羊座", u8"金牛座", u8"双子座", u8"巨蟹座", u8"狮子座", u8"处女座",
    u8"天秤座", u8"天蝎座", u8"射手座", u8"摩羯座", u8"水瓶座", u8"双鱼座",
};
constexpr int kSignCount = static_cast<int>(sizeof(kSigns) / sizeof(kSigns[0]));

constexpr const char* kRatings[] = {u8"运势极佳", u8"稳中有升", u8"平稳过渡", u8"略有起伏", u8"宜低调行事"};
constexpr const char* kOverall[] = {
    u8"今日心情不错，适合挑战一件一直想做的事。",
    u8"与人合作会带来惊喜，主动一点更容易成事。",
    u8"放慢节奏，享受当下，一些小事反而让人安心。",
    u8"计划赶不上变化，随机应变比硬撑更聪明。",
    u8"适合整理和规划，把堆积的问题一件件解决。",
    u8"保持乐观，贵人往往在你不经意时出现。",
    u8"少说多做，今日的行动比语言更有力量。",
    u8"给自己留点独处的时间，灵感会在安静中到来。",
    u8"财运平稳，理性消费，别被一时冲动左右。",
    u8"沟通顺畅，适合谈合作与修复关系。",
};
constexpr int kRatingCount = static_cast<int>(sizeof(kRatings) / sizeof(kRatings[0]));
constexpr int kOverallCount = static_cast<int>(sizeof(kOverall) / sizeof(kOverall[0]));

}  // namespace

void HoroscopeActivity::onEnter() {
  Activity::onEnter();
  roll();
}

void HoroscopeActivity::roll() {
  rating_ = kRatings[esp_random() % kRatingCount];
  overall_ = kOverall[esp_random() % kOverallCount];
  requestUpdate();
}

void HoroscopeActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goToApps();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    signIndex_ = (signIndex_ + kSignCount - 1) % kSignCount;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    signIndex_ = (signIndex_ + 1) % kSignCount;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    roll();
  }
}

void HoroscopeActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_HOROSCOPE_TITLE));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 40;
  const int contentLeft = metrics.contentSidePadding;

  renderer.drawText(UI_12_FONT_ID, contentLeft, contentTop + 8, kSigns[signIndex_], true, EpdFontFamily::BOLD);
  renderer.drawText(SMALL_FONT_ID, contentLeft, contentTop + 8 + renderer.getLineHeight(UI_12_FONT_ID), rating_, true,
                    EpdFontFamily::REGULAR);

  const int boxY = contentTop + renderer.getLineHeight(UI_12_FONT_ID) + renderer.getLineHeight(SMALL_FONT_ID) + metrics.verticalSpacing + 10;
  const int boxW = pageWidth - 2 * contentLeft;
  const int boxH = 120;
  renderer.fillRoundedRect(contentLeft, boxY, boxW, boxH, metrics.controlRadius, Color::White);
  renderer.drawRoundedRect(contentLeft, boxY, boxW, boxH, 2, metrics.controlRadius, true);

  const int wrapW = boxW - 2 * metrics.verticalSpacing;
  const auto lines = renderer.wrappedText(UI_12_FONT_ID, overall_, wrapW, 4, EpdFontFamily::REGULAR);
  int y = boxY + metrics.verticalSpacing;
  for (const auto& line : lines) {
    renderer.drawText(UI_12_FONT_ID, contentLeft + metrics.verticalSpacing, y, line.c_str(), true,
                      EpdFontFamily::REGULAR);
    y += renderer.getLineHeight(UI_12_FONT_ID);
  }

  const auto labels =
      mappedInput.mapLabels(tr(STR_BACK), tr(STR_HOROSCOPE_ROLL), tr(STR_HOROSCOPE_PREV), tr(STR_HOROSCOPE_NEXT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}