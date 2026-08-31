#include "BookAnswersActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <esp_system.h>

#include "components/UITheme.h"
#include "fontIds.h"

namespace {

constexpr const char* kAnswers[] = {
    "毫无疑问。",       "可以，大胆去做。",  "再想一想。",     "不要犹豫，试试看。",
    "现在还不是时候。", "相信你的直觉。",    "结果会很好。",   "也许该换个角度。",
    "答案已有，别急。", "坚持下去会有回报。", "风险略大，谨慎为上。", "机会就在眼前。",
    "顺其自然即可。", "放下执念，随缘。",    "是的，马上行动。", "别问那么多，先做。",
    "心之所向，素履以往。", "此路不通，换一条。", "让时间给出答案。", "你心里其实已经有数。",
};
constexpr int kAnswerCount = static_cast<int>(sizeof(kAnswers) / sizeof(kAnswers[0]));

int randPage() { return 1 + static_cast<int>(esp_random() % 300); }

}  // namespace

void BookAnswersActivity::onEnter() {
  Activity::onEnter();
  ask();
}

void BookAnswersActivity::ask() {
  answer_ = kAnswers[esp_random() % kAnswerCount];
  page_ = randPage();
  // Page-turn reveal: briefly flip through pages before the answer lands.
  revealing_ = true;
  revealUntilMs_ = millis() + 650;
  animMs_ = millis();
  animPage_ = page_;
  requestUpdate();
}

void BookAnswersActivity::loop() {
  const unsigned long now = millis();
  if (revealing_) {
    // Flip the page number upward to sell the "turning pages" illusion.
    if (now - animMs_ >= 90) {
      animMs_ = now;
      animPage_ = (animPage_ + (1 + static_cast<int>(esp_random() % 25))) % 400 + 1;
      requestUpdate();
    }
    if (now >= revealUntilMs_) {
      revealing_ = false;
      requestUpdate();
    }
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
    ask();
  }
}

void BookAnswersActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_BOOK_ANSWERS_TITLE));

  const int cardW = pageWidth - 2 * metrics.contentSidePadding;
  const int cardH = 240;
  const int marginTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 60;
  const int cx = (pageWidth - cardW) / 2;
  renderer.fillRoundedRect(cx, marginTop, cardW, cardH, metrics.controlRadius, Color::White);
  renderer.drawRoundedRect(cx, marginTop, cardW, cardH, 2, metrics.controlRadius, true);

  char pageText[8];
  snprintf(pageText, sizeof(pageText), "%d", revealing_ ? animPage_ : page_);
  renderer.drawCenteredText(SMALL_FONT_ID, marginTop + 30, pageText, false, EpdFontFamily::REGULAR);

  // During the reveal we show the "turning pages" message; once it lands, show
  // the oracular answer with a subtle underline flourish.
  const int answerY = marginTop + cardH / 2 - renderer.getLineHeight(UI_12_FONT_ID) / 2;
  if (revealing_) {
    renderer.drawCenteredText(UI_12_FONT_ID, answerY, tr(STR_BOOK_ANSWERS_OPENING), true, EpdFontFamily::BOLD);
  } else {
    renderer.drawCenteredText(UI_12_FONT_ID, answerY, answer_ ? answer_ : "", true, EpdFontFamily::BOLD);
    const int answerW = renderer.getTextWidth(UI_12_FONT_ID, answer_ ? answer_ : "", EpdFontFamily::BOLD);
    renderer.drawLine(cx + (cardW - answerW) / 2, answerY + renderer.getLineHeight(UI_12_FONT_ID), cx + (cardW + answerW) / 2,
                      answerY + renderer.getLineHeight(UI_12_FONT_ID), 2, true);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_BOOK_ANSWERS_RETURN), "", tr(STR_BOOK_ANSWERS_RETURN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}