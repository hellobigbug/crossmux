#include "BookAnswersActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <esp_system.h>

#include "components/UITheme.h"
#include "fontIds.h"

namespace {

constexpr const char* kAnswers[] = {
    u8"毫无疑问。",       u8"可以，大胆去做。",  u8"再想一想。",     u8"不要犹豫，试试看。",
    u8"现在还不是时候。", u8"相信你的直觉。",    u8"结果会很好。",   u8"也许该换个角度。",
    u8"答案已有，别急。", u8"坚持下去会有回报。", u8"风险略大，谨慎为上。", u8"机会就在眼前。",
    u8"顺其自然即可。", u8"放下执念，随缘。",    u8"是的，马上行动。", u8"别问那么多，先做。",
    u8"心之所向，素履以往。", u8"此路不通，换一条。", u8"让时间给出答案。", u8"你心里其实已经有数。",
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
  requestUpdate();
}

void BookAnswersActivity::loop() {
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

  char answer[8];
  snprintf(answer, sizeof(answer), "%d", page_);
  renderer.drawCenteredText(SMALL_FONT_ID, marginTop + 30, answer, false, EpdFontFamily::REGULAR);

  const int answerY = marginTop + cardH / 2 - renderer.getLineHeight(UI_12_FONT_ID) / 2;
  renderer.drawCenteredText(UI_12_FONT_ID, answerY, answer_ ? answer_ : "", true, EpdFontFamily::BOLD);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_BOOK_ANSWERS_RETURN), "", tr(STR_BOOK_ANSWERS_RETURN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}