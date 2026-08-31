#include "HoroscopeActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <esp_system.h>

#include "activities/apps/GameUi.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {

constexpr const char* kSigns[] = {
    "白羊座", "金牛座", "双子座", "巨蟹座", "狮子座", "处女座",
    "天秤座", "天蝎座", "射手座", "摩羯座", "水瓶座", "双鱼座",
};
constexpr int kSignCount = static_cast<int>(sizeof(kSigns) / sizeof(kSigns[0]));

constexpr const char* kRatings[] = {"运势极佳", "稳中有升", "平稳过渡", "略有起伏", "宜低调行事"};
constexpr const char* kOverall[] = {
    "今日心情不错，适合挑战一件一直想做的事。",
    "与人合作会带来惊喜，主动一点更容易成事。",
    "放慢节奏，享受当下，一些小事反而让人安心。",
    "计划赶不上变化，随机应变比硬撑更聪明。",
    "适合整理和规划，把堆积的问题一件件解决。",
    "保持乐观，贵人往往在你不经意时出现。",
    "少说多做，今日的行动比语言更有力量。",
    "给自己留点独处的时间，灵感会在安静中到来。",
    "财运平稳，理性消费，别被一时冲动左右。",
    "沟通顺畅，适合谈合作与修复关系。",
};
constexpr int kRatingCount = static_cast<int>(sizeof(kRatings) / sizeof(kRatings[0]));
constexpr int kOverallCount = static_cast<int>(sizeof(kOverall) / sizeof(kOverall[0]));

constexpr const char* kLuckyColors[] = {"金色", "红色", "蓝色", "绿色", "紫色", "米白", "橙色", "天蓝"};
constexpr const char* kFavorable[] = {"出行", "会见朋友", "读书", "运动", "谈判", "整理", "表白", "面试"};
constexpr const char* kAvoid[] = {"熬夜", "冲动消费", "争执", "冒险", "口舌之争", "过度食用甜食", "久坐不动", "轻信他人"};
constexpr int kLuckyColorCount = static_cast<int>(sizeof(kLuckyColors) / sizeof(kLuckyColors[0]));
constexpr int kFavorableCount = static_cast<int>(sizeof(kFavorable) / sizeof(kFavorable[0]));
constexpr int kAvoidCount = static_cast<int>(sizeof(kAvoid) / sizeof(kAvoid[0]));

}  // namespace

void HoroscopeActivity::onEnter() {
  Activity::onEnter();
  roll();
}

void HoroscopeActivity::roll() {
  rating_ = kRatings[esp_random() % kRatingCount];
  overall_ = kOverall[esp_random() % kOverallCount];
  luckyNum_ = 1 + esp_random() % 99;
  luckyColor_ = kLuckyColors[esp_random() % kLuckyColorCount];
  favorable_ = kFavorable[esp_random() % kFavorableCount];
  avoid_ = kAvoid[esp_random() % kAvoidCount];
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

  // Lucky details row: number + color drawn alongside their labels.
  char numStr[8];
  snprintf(numStr, sizeof(numStr), "%d", luckyNum_);
  const int luckyRowY = boxY + boxH + metrics.verticalSpacing + 8;
  renderer.drawText(UI_12_FONT_ID, contentLeft, luckyRowY, tr(STR_HOROSCOPE_LUCKY_NUM), true, EpdFontFamily::BOLD);
  const int numPrefixW = renderer.getTextWidth(UI_12_FONT_ID, tr(STR_HOROSCOPE_LUCKY_NUM), EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, contentLeft + numPrefixW, luckyRowY, numStr, true, EpdFontFamily::REGULAR);

  const char* color = luckyColor_ ? luckyColor_ : "";
  const int colorX = contentLeft + numPrefixW + renderer.getTextWidth(UI_12_FONT_ID, numStr, EpdFontFamily::REGULAR) + 18;
  renderer.drawText(UI_12_FONT_ID, colorX, luckyRowY, tr(STR_HOROSCOPE_LUCKY_COLOR), true, EpdFontFamily::BOLD);
  const int colorPrefixW = renderer.getTextWidth(UI_12_FONT_ID, tr(STR_HOROSCOPE_LUCKY_COLOR), EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, colorX + colorPrefixW, luckyRowY, color, true, EpdFontFamily::REGULAR);

  // 宜 / 忌 chips (prefix bold, value regular on the same row).
  const char* fav = favorable_ ? favorable_ : "";
  const char* avo = avoid_ ? avoid_ : "";

  const int chipY = luckyRowY + renderer.getLineHeight(UI_12_FONT_ID) + 6;
  renderer.drawText(UI_12_FONT_ID, contentLeft, chipY, tr(STR_HOROSCOPE_FAVORABLE), true, EpdFontFamily::BOLD);
  const int favPrefixW = renderer.getTextWidth(UI_12_FONT_ID, tr(STR_HOROSCOPE_FAVORABLE), EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, contentLeft + favPrefixW, chipY, fav, true, EpdFontFamily::REGULAR);

  const int avoidY = chipY + renderer.getLineHeight(UI_12_FONT_ID) + 4;
  renderer.drawText(UI_12_FONT_ID, contentLeft, avoidY, tr(STR_HOROSCOPE_AVOID), true, EpdFontFamily::BOLD);
  const int avoPrefixW = renderer.getTextWidth(UI_12_FONT_ID, tr(STR_HOROSCOPE_AVOID), EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, contentLeft + avoPrefixW, avoidY, avo, true, EpdFontFamily::REGULAR);

  const auto labels =
      mappedInput.mapLabels(tr(STR_BACK), tr(STR_HOROSCOPE_ROLL), tr(STR_HOROSCOPE_PREV), tr(STR_HOROSCOPE_NEXT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}