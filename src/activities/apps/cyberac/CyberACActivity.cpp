#include "CyberACActivity.h"

#include <Arduino.h>
#include <I18n.h>

#include "components/UITheme.h"
#include "fontIds.h"

namespace {

constexpr const char* kModes[] = {"制冷", "制热", "送风", "除湿"};
constexpr int kModeCount = static_cast<int>(sizeof(kModes) / sizeof(kModes[0]));

constexpr int kMinTemp = 16;
constexpr int kMaxTemp = 30;
constexpr int kBodyW = 260;
constexpr int kBodyH = 150;
constexpr int kBodyRadius = 22;

}  // namespace

void CyberACActivity::togglePower() {
  on_ = !on_;
  if (on_) {
    // Power-on reveal: temps flash outline for ~startup period before lighting.
    booting_ = true;
    bootUntilMs_ = millis() + 800;
    phase_ = 0;
  } else {
    booting_ = false;
  }
  requestUpdate();
}

void CyberACActivity::loop() {
  const unsigned long now = millis();

  // Airflow animation: advance the phase so the stream looks like it breathes.
  if (on_ && now - animMs_ >= 170) {
    animMs_ = now;
    ++phase_;
    requestUpdate();
  }
  if (booting_ && now >= bootUntilMs_) {
    booting_ = false;
    requestUpdate();
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goToApps();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    togglePower();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Up) && on_) {
    temp_ = (temp_ >= kMaxTemp) ? kMaxTemp : temp_ + 1;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down) && on_) {
    temp_ = (temp_ <= kMinTemp) ? kMinTemp : temp_ - 1;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Left) && on_) {
    mode_ = (mode_ - 1 + kModeCount) % kModeCount;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Right) && on_) {
    mode_ = (mode_ + 1) % kModeCount;
    requestUpdate();
  }
}

void CyberACActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_CYBER_AC_TITLE));

  const int bodyX = (pageWidth - kBodyW) / 2;
  const int bodyY = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 30;

  const bool lit = on_;
  // AC unit outline.
  renderer.drawRoundedRect(bodyX, bodyY, kBodyW, kBodyH, 2, kBodyRadius, true);
  // Vents (horizontal lines across the lower half).
  const int ventY0 = bodyY + kBodyH * 5 / 8;
  const int ventY1 = bodyY + kBodyH - 12;
  for (int i = 0; i < 4; ++i) {
    const int y = ventY0 + ((ventY1 - ventY0) * i) / 3;
    renderer.drawLine(bodyX + 12, y, bodyX + kBodyW - 12, y, 1, lit);
  }
  // "Air stream" arcs billowing out to the right when on. The phase nudges the
  // stream so it looks like it is continuously flowing.
  if (lit) {
    const int flow = phase_ % 3;
    for (int i = 0; i < 3; ++i) {
      const int cy = bodyY + kBodyH * 5 / 8 + ((ventY1 - ventY0) * i) / 3;
      const int reach = 10 + flow * 6 + (i == 1 ? 4 : 0);
      renderer.drawArc(kBodyW / 2 + reach, bodyX + kBodyW - 10, cy, 1, 0, 1, true);
      renderer.drawLine(bodyX + kBodyW - 10 + reach + 6, cy, bodyX + kBodyW - 10 + reach + 14, cy, 1, true);
    }
  }
  // Power LED in the top-right corner of the unit.
  const int ledX = bodyX + kBodyW - 18;
  const int ledY = bodyY + 14;
  renderer.drawArc(7, ledX, ledY, 1, -1, 1, true);
  if (lit) renderer.fillRoundedRect(ledX - 5, ledY - 5, 10, 10, 5, Color::Black);

  // Status / temperature readout in the top band. During power-on it is drawn
  // inverted ("off") so it appears to flash on.
  char tempText[8];
  snprintf(tempText, sizeof(tempText), "%d°C", temp_);
  const bool litReadout = lit && !booting_;
  renderer.drawCenteredText(UI_12_FONT_ID, bodyY + 14, tempText, litReadout, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, bodyY + 14 + renderer.getLineHeight(UI_12_FONT_ID), kModes[mode_], litReadout,
                            EpdFontFamily::REGULAR);

  // Remote button row (drawn controls mirror the physical button mapping).
  constexpr int kBtnW = 64;
  constexpr int kBtnH = 54;
  const int gap = metrics.menuSpacing;
  const int rowW = 3 * kBtnW + 2 * gap;
  const int rowX = (pageWidth - rowW) / 2;
  const int rowY = pageHeight - metrics.buttonHintsHeight - kBtnH - metrics.verticalSpacing - 30;
#ifdef CROSSPOINT_EMULATED
  (void)rowX;
#endif
  const char* labelsC[3] = {tr(STR_CYBER_AC_POWER_LABEL), tr(STR_CYBER_AC_MODE_LABEL), tr(STR_CYBER_AC_TEMP_LABEL)};
  for (int i = 0; i < 3; ++i) {
    const int x = rowX + i * (kBtnW + gap);
    renderer.drawRoundedRect(x, rowY, kBtnW, kBtnH, 2, metrics.controlRadius, true);
    renderer.drawCenteredText(SMALL_FONT_ID, rowY + kBtnH / 2 - renderer.getLineHeight(SMALL_FONT_ID) / 2, labelsC[i], true,
                              EpdFontFamily::REGULAR);
  }

  const auto labels =
      mappedInput.mapLabels(tr(STR_BACK), lit ? tr(STR_CYBER_AC_OFF_LABEL) : tr(STR_CYBER_AC_ON_LABEL),
                            tr(STR_CYBER_AC_MODE_LABEL), tr(STR_CYBER_AC_TEMP_LABEL));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}