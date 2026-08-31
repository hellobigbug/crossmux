#include "NokiaTheme.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <string>

#include "CrossPointSettings.h"
#include "fontIds.h"

namespace {

// Big font constants. On non-CJK builds we use the larger Noto Sans sizes for
// the Nokia "bold soft-key" look; CJK builds keep the 12px UI font so the
// built-in CJK fallback still covers Chinese labels (there is no 16px UI CJK
// font, and rendering tofu in the default theme would be a regression).
#ifdef ENABLE_CHINESE_VERSION
constexpr int kTitleFontId = UI_12_FONT_ID;
constexpr int kGuideFontId = UI_10_FONT_ID;
#else
constexpr int kTitleFontId = NOTOSANS_16_FONT_ID;
constexpr int kGuideFontId = NOTOSANS_14_FONT_ID;
#endif

// Bottom-to-top clearance inside each soft-key / list row so the glyphs sit
// nicely centered with plenty of white space (the "big target" feel).
constexpr int kTapPaddingY = 24;
// Rounded corner radius for the Nokia soft-key buttons.
constexpr int kKeyRadius = 30;
constexpr int kRowRadius = 24;

void drawNokiaScrollBar(const GfxRenderer& renderer, Rect rect, int itemCount, int pageStartIndex, int pageItems) {
  if (itemCount <= 0 || pageItems <= 0 || itemCount <= pageItems) {
    return;
  }
  const int barW = NokiaMetrics::values.scrollBarWidth;
  const int barX = rect.x + rect.width - NokiaMetrics::values.scrollBarRightOffset - barW;
  const int barH = rect.height;
  const int thumbH = std::max(10, (barH * pageItems) / itemCount);
  const int maxStart = std::max(1, itemCount - pageItems);
  const int maxTravel = std::max(1, barH - thumbH);
  const int clampedStart = std::clamp(pageStartIndex, 0, maxStart);
  const int thumbY = rect.y + (clampedStart * maxTravel) / maxStart;
  renderer.fillRect(barX, thumbY, barW, thumbH);
}

}  // namespace

void NokiaTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                            const char* subtitle) const {
  (void)subtitle;
  // Home screen header is custom-rendered in drawRecentBookCover (inherited).
  if (title == nullptr) {
    return;
  }
  const int sidePadding = NokiaMetrics::values.contentSidePadding;
  const int titleX = rect.x + sidePadding;
  const int titleY = rect.y + 16;

  const bool showBatteryPercentage =
      SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS;
  const int batteryIconX = rect.x + rect.width - sidePadding - NokiaMetrics::values.batteryWidth;

  int batteryGroupLeftX = batteryIconX;
  if (showBatteryPercentage) {
    const int maxTextWidth = renderer.getTextWidth(STATUS_NUMERIC_FONT_ID, "100%");
    const int clearW = maxTextWidth + batteryPercentSpacing + NokiaMetrics::values.batteryWidth;
    const int clearH =
        std::max(renderer.getTextHeight(STATUS_NUMERIC_FONT_ID),
                 NokiaMetrics::values.batteryHeight + 8);
    renderer.fillRect(batteryIconX - maxTextWidth - batteryPercentSpacing, rect.y + 12, clearW, clearH, false);
    batteryGroupLeftX = batteryIconX - maxTextWidth - batteryPercentSpacing;
  }

  const int maxTitleWidth = std::max(0, batteryGroupLeftX - 20 - titleX);
  auto headerTitle = renderer.truncatedText(kTitleFontId, title, maxTitleWidth, EpdFontFamily::BOLD);
  renderer.drawText(kTitleFontId, titleX, titleY, headerTitle.c_str(), true, EpdFontFamily::BOLD);
  drawBatteryRight(renderer,
                   Rect{batteryIconX, rect.y + 14, NokiaMetrics::values.batteryWidth,
                        NokiaMetrics::values.batteryHeight},
                   showBatteryPercentage);
}

int NokiaTheme::getMenuRowHeight(const GfxRenderer& renderer) const {
  return renderer.getLineHeight(kTitleFontId) + 2 * kTapPaddingY;
}

void NokiaTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                const std::function<std::string(int index)>& buttonLabel,
                                const std::function<UIIcon(int index)>& rowIcon, int rowSpacing) const {
  (void)rowIcon;
  const int sidePadding = NokiaMetrics::values.contentSidePadding;
  const int rowX = rect.x + sidePadding;
  const int rowWidth = rect.width - sidePadding * 2;
  const int rowHeight = getMenuRowHeight(renderer);
  const int rowGap = rowSpacing >= 0 ? rowSpacing : NokiaMetrics::values.menuSpacing;
  const int rowStep = rowHeight + rowGap;
  const int textLineHeight = renderer.getLineHeight(kTitleFontId);
  const int pageItems = std::max(1, rect.height / rowStep);
  const int safeSelectedIndex = std::max(0, selectedIndex);
  const int pageStartIndex = (safeSelectedIndex / pageItems) * pageItems;
  const int menuTop = rect.y;

  for (int i = pageStartIndex; i < buttonCount && i < pageStartIndex + pageItems; ++i) {
    const auto label = buttonLabel(i);
    const std::string truncatedLabel =
        renderer.truncatedText(kTitleFontId, label.c_str(), std::max(0, rowWidth - 24),
                               EpdFontFamily::BOLD);
    const int rowY = menuTop + (i - pageStartIndex) * rowStep;
    const bool isSelected = selectedIndex == i;
    // Full-width rounded "soft key"; selected is inverted (white on black).
    renderer.fillRoundedRect(rowX, rowY, rowWidth, rowHeight, kKeyRadius,
                             isSelected ? Color::Black : Color::White);
    const int textW = renderer.getTextWidth(kTitleFontId, truncatedLabel.c_str(), EpdFontFamily::BOLD);
    const int textX = rect.x + (rect.width - textW) / 2;
    const int textY = rowY + (rowHeight - textLineHeight) / 2;
    renderer.drawText(kTitleFontId, textX, textY, truncatedLabel.c_str(), !isSelected, EpdFontFamily::BOLD);
  }

  drawNokiaScrollBar(renderer, rect, buttonCount, pageStartIndex, pageItems);
}

int NokiaTheme::getListRowStep(bool hasSubtitle) const {
  const int rowHeight =
      hasSubtitle ? NokiaMetrics::values.listWithSubtitleRowHeight : NokiaMetrics::values.listRowHeight;
  return rowHeight + NokiaMetrics::values.listRowGap;
}

int NokiaTheme::getListPageItems(int contentHeight, bool hasSubtitle) const {
  return std::max(1, contentHeight / getListRowStep(hasSubtitle));
}

void NokiaTheme::drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                          const std::function<std::string(int index)>& rowTitle,
                          const std::function<std::string(int index)>& rowSubtitle,
                          const std::function<UIIcon(int index)>& rowIcon,
                          const std::function<std::string(int index)>& rowValue, bool highlightValue,
                          const std::function<bool(int index)>& rowDimmed, const bool showSelection,
                          const std::function<bool(int index)>&) const {
  (void)rowIcon;
  (void)highlightValue;
  const bool hasSubtitle = static_cast<bool>(rowSubtitle);
  const int titleLineHeight = renderer.getLineHeight(kTitleFontId);
  const int subtitleLineHeight = renderer.getLineHeight(SMALL_FONT_ID);
  constexpr int subtitleTopPadding = 12;
  constexpr int subtitleBottomPadding = 10;
  constexpr int subtitleInterLineGap = 4;
  const int subtitleRowHeight =
      subtitleTopPadding + titleLineHeight + subtitleInterLineGap + subtitleLineHeight +
      subtitleBottomPadding;
  const int rowHeight = hasSubtitle ? subtitleRowHeight : NokiaMetrics::values.listRowHeight;
  const int rowStep = rowHeight + NokiaMetrics::values.listRowGap;
  const int pageItems = std::max(1, rect.height / rowStep);
  const int pageStartIndex = std::max(0, selectedIndex / pageItems) * pageItems;

  const int sidePadding = NokiaMetrics::values.contentSidePadding;
  const int rowX = rect.x + sidePadding;
  const int rowWidth = rect.width - sidePadding * 2;

  for (int i = pageStartIndex; i < itemCount && i < pageStartIndex + pageItems; i++) {
    const int rowY = rect.y + (i % pageItems) * rowStep;
    const bool isSelected = showSelection && i == selectedIndex;
    const bool dimmed = rowDimmed && rowDimmed(i) && !isSelected;
    renderer.fillRoundedRect(rowX, rowY, rowWidth, rowHeight, kRowRadius,
                             isSelected ? Color::Black : Color::White);

    constexpr int kItemInsetX = 20;
    constexpr int kMinTitleWidth = 40;
    constexpr int kMinValueGap = 20;
    int textAreaWidth = rowWidth - kItemInsetX * 2;
    if (rowValue) {
      std::string valueText = rowValue(i);
      if (!valueText.empty()) {
        const int maxValueWidth = std::max(0, rowWidth - kItemInsetX * 2 - kMinValueGap - kMinTitleWidth);
        if (maxValueWidth > 0) {
          const std::string truncatedValue =
              renderer.truncatedText(kTitleFontId, valueText.c_str(), maxValueWidth, EpdFontFamily::REGULAR);
          const int valueW = renderer.getTextWidth(kTitleFontId, truncatedValue.c_str(), EpdFontFamily::REGULAR);
          const int valueX = rowX + rowWidth - kItemInsetX - valueW;
          const int valueY = rowY + (rowHeight - renderer.getLineHeight(kTitleFontId)) / 2;
          renderer.drawText(kTitleFontId, valueX, valueY, truncatedValue.c_str(), !isSelected,
                            EpdFontFamily::REGULAR);
          textAreaWidth = std::max(0, textAreaWidth - valueW - kMinValueGap);
        }
      }
    }

    if (hasSubtitle) {
      const std::string subtitleRaw = rowSubtitle(i);
      auto title = renderer.truncatedText(kTitleFontId, rowTitle(i).c_str(), textAreaWidth, EpdFontFamily::BOLD);
      if (subtitleRaw.empty()) {
        const int centeredTitleY = rowY + (rowHeight - titleLineHeight) / 2;
        renderer.drawText(kTitleFontId, rowX + kItemInsetX, centeredTitleY, title.c_str(), !isSelected,
                          EpdFontFamily::BOLD);
      } else {
        const int titleY = rowY + subtitleTopPadding;
        const int subtitleY = titleY + titleLineHeight + subtitleInterLineGap;
        auto subtitle =
            renderer.truncatedText(SMALL_FONT_ID, subtitleRaw.c_str(), textAreaWidth, EpdFontFamily::REGULAR);
        renderer.drawText(kTitleFontId, rowX + kItemInsetX, titleY, title.c_str(), !isSelected,
                          EpdFontFamily::BOLD);
        renderer.drawText(SMALL_FONT_ID, rowX + kItemInsetX, subtitleY, subtitle.c_str(), !isSelected,
                          EpdFontFamily::REGULAR);
      }
    } else {
      auto title = renderer.truncatedText(kTitleFontId, rowTitle(i).c_str(), textAreaWidth, EpdFontFamily::BOLD);
      const int titleY = rowY + (rowHeight - titleLineHeight) / 2;
      renderer.drawText(kTitleFontId, rowX + kItemInsetX, titleY, title.c_str(), !isSelected,
                        EpdFontFamily::BOLD);
    }
  }

  drawNokiaScrollBar(renderer, rect, itemCount, pageStartIndex, pageItems);
}

void NokiaTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                                 const char* btn4) const {
  if (!buttonHintsVisible()) {
    return;
  }

  const GfxRenderer::Orientation origOrientation = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int sidePadding = NokiaMetrics::values.contentSidePadding;
  const int groupGap = 12;
  const int bottomMargin = 12;
  const int hintHeight = NokiaMetrics::values.buttonHintsHeight - 12;
  const int groupWidth = (pageWidth - sidePadding * 2 - groupGap) / 2;
  const int hintY = pageHeight - hintHeight - bottomMargin;
  const int textLineHeight = renderer.getLineHeight(kGuideFontId);
  const int textY = hintY + (hintHeight - textLineHeight) / 2;

  const bool backDisabled = (btn1 == nullptr || btn1[0] == '\0');
  const std::string backLabel = backDisabled ? "" : std::string(btn1);
  const std::string selectText = (btn2 && btn2[0] != '\0') ? std::string(btn2) : "";
  const std::string upText = (btn3 && btn3[0] != '\0') ? std::string(btn3) : "";
  const std::string downText = (btn4 && btn4[0] != '\0') ? std::string(btn4) : "";

  const int leftGroupX = sidePadding;
  const int rightGroupX = leftGroupX + groupWidth + groupGap;

  // Clear any prior content in the guide band, then draw two rounded soft keys.
  renderer.fillRect(leftGroupX, hintY, groupWidth, hintHeight, false);
  renderer.fillRect(rightGroupX, hintY, groupWidth, hintHeight, false);

  renderer.drawRoundedRect(leftGroupX, hintY, groupWidth, hintHeight, 2, 16, true);
  renderer.drawRoundedRect(rightGroupX, hintY, groupWidth, hintHeight, 2, 16, true);

  const int leftLabelsWidth =
      renderer.getTextWidth(kGuideFontId, backLabel.c_str(), EpdFontFamily::REGULAR) +
      renderer.getTextWidth(kGuideFontId, selectText.c_str(), EpdFontFamily::REGULAR) + 4;
  int backX = leftGroupX + (groupWidth / 2) - leftLabelsWidth / 2;
  if (!backDisabled) {
    renderer.drawText(kGuideFontId, backX, textY, backLabel.c_str(), true, EpdFontFamily::REGULAR);
    backX += renderer.getTextWidth(kGuideFontId, backLabel.c_str(), EpdFontFamily::REGULAR) + 4;
  }
  renderer.drawText(kGuideFontId, backX, textY, selectText.c_str(), true, EpdFontFamily::REGULAR);

  const int rightLabelsWidth =
      renderer.getTextWidth(kGuideFontId, upText.c_str(), EpdFontFamily::REGULAR) +
      renderer.getTextWidth(kGuideFontId, downText.c_str(), EpdFontFamily::REGULAR) + 4;
  int upX = rightGroupX + (groupWidth / 2) - rightLabelsWidth / 2;
  renderer.drawText(kGuideFontId, upX, textY, upText.c_str(), true, EpdFontFamily::REGULAR);
  upX += renderer.getTextWidth(kGuideFontId, upText.c_str(), EpdFontFamily::REGULAR) + 4;
  renderer.drawText(kGuideFontId, upX, textY, downText.c_str(), true, EpdFontFamily::REGULAR);

  renderer.setOrientation(origOrientation);
}