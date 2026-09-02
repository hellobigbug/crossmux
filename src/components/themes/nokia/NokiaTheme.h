#pragma once

#include "components/themes/BaseTheme.h"
#include "components/themes/roundedraff/RoundedRaffTheme.h"

namespace NokiaMetrics {
// Nokia retro layout tuned for the small-screen (768x552) EEGO A4: big touch
// targets and big text while everything stays inside the reduced height.
constexpr ThemeMetrics values = {.batteryWidth = 15,
                                 .batteryHeight = 12,
                                 .topPadding = 4,
                                 .batteryBarHeight = 20,
                                 // Taller nav bar: roomier back button and
                                 // more air before the content band.
                                 .headerHeight = 58,
                                 .verticalSpacing = 10,
                                 .previewPadding = 12,
                                 .previewHeightPercent = 30,
                                 .contentSidePadding = 24,
                                 .touchEdgeInset = 20,
                                 .listRowHeight = 58,
                                 .listWithSubtitleRowHeight = 84,
                                 .listRowGap = 8,
                                 // Rounded-rectangle rows (radius < half the
                                 // row height) instead of an oval pill.
                                 .listRowRadius = 10,
                                 .listInset = 0,
                                 .listSidePadding = 20,
                                 .listSelectionStyle = 0,  // invert fill
                                 .listScrollWidth = 4,
                                 .listScrollSide = 0,
                                 .listTitleBold = true,
                                 .listSeparatorStyle = 2,  // dotted divider
                                 .listValueMaxWidth = 0,
                                 .listSelectionCoversScrollReservation = false,
                                 .headerSidePadding = 20,
                                 .headerUnderlineSize = 0,
                                 .headerTitleAlign = 1,  // centered
                                 .headerBatterySide = 0,
                                 .headerBatteryDetached = false,
                                 .menuRowHeight = 68,
                                 .menuSpacing = 12,
                                 .tabSpacing = 12,
                                 .tabBarHeight = 58,
                                 .tabPillFullSlot = true,
                                 .scrollBarWidth = 4,
                                 .scrollBarRightOffset = 5,
                                 .homeTopPadding = 60,
                                 .homeCoverHeight = 150,
                                 // The cover band hosts the "bookshelf + clock"
                                 // module above the nine-grid, so it reserves a
                                 // real module height instead of a cover card.
                                 // NokiaTheme::getHomeModuleHeight adapts this
                                 // to the square menu grid; 250 is the cap.
                                 .homeCoverTileHeight = 250,
                                 // Two book cards side by side in the module.
                                 .homeRecentBooksCount = 2,
                                 .homeShowRecentBookTitle = false,
                                 .homeContinueReadingInMenu = true,
                                 .homeMenuTopOffset = 16,
                                 .buttonHintsHeight = 62,
                                 .sideButtonHintsWidth = 40,
                                 .progressBarHeight = 16,
                                 .progressBarMarginTop = 1,
                                 .statusBarHorizontalMargin = 5,
                                 .statusBarVerticalMargin = 19,
                                 .keyboardKeyHeight = 60,
                                 .keyboardKeySpacing = 12,
                                 .keyboardCenteredText = true,
                                 .keyboardVerticalOffset = 0,
                                 .keyboardTextFieldWidthPercent = 88,
                                 .keyboardWidthPercent = 96,
                                 .popupTopOffsetRatio = 0.12f,
                                 .popupMarginX = 22,
                                 .popupMarginY = 16,
                                 .popupFrameThickness = 2,
                                 .popupCornerRadius = 20,
                                 .popupTextBold = true,
                                 .popupTextInverted = false,
                                 .popupTextBaselineOffsetY = -2,
                                 .popupProgressBarHeight = 4,
                                 .popupProgressDrawOutline = true,
                                 .popupProgressClampPercent = true,
                                 .popupProgressFillInverted = false,
                                 .popupProgressOutlineInverted = false,
                                 .optionPopupItemSpacing = 8,
                                 .optionPopupInnerPadding = 26,
                                 .optionPopupSelectionHPadding = 22,
                                 .optionPopupSelectionVPadding = 12,
                                 .optionPopupTitleGap = 18,
                                 .optionPopupUseSmallFont = false,
                                 .optionPopupOptionFontBold = true,
                                 .optionPopupSelectionRadius = 12,
                                 .optionPopupSelectionLight = false,
                                 .optionPopupDrawAllRows = true,
                                 .optionPopupDialogSideMargin = 22,
                                 .optionPopupTitleSeparator = true,
                                 .textFieldHorizontalPadding = 8,
                                 .textFieldNormalThickness = 2,
                                 .textFieldCursorThickness = 3,
                                 .textFieldLineEndOffset = -1,
                                 .controlRadius = 24,
                                 .sheetRadius = 22,
                                 .capsuleRadius = 255};
}  // namespace NokiaMetrics

// Nokia retro theme: large rounded "soft-key" buttons and large, heavy text,
// adapted to the small EEGO A4 panel. Reuses RoundedRaffTheme's rounded card
// rendering and overrides the high-touch-density surfaces with bolder sizes.
class NokiaTheme : public RoundedRaffTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                  const char* subtitle = nullptr) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon, int rowSpacing = -1) const override;
  // Retro Nokia main-screen grid: 3 columns of rounded soft-key tiles with a
  // 32px icon (drawn at 2x) and a bold label; the selected tile inverts.
  void drawHomeMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                    const std::function<std::string(int index)>& buttonLabel,
                    const std::function<UIIcon(int index)>& rowIcon) const override;
  HomeGridLayout getHomeGridLayout(const GfxRenderer& renderer, int itemCount) const override;
  int getHomeModuleHeight(const GfxRenderer& renderer, int itemCount) const override;
  HomeCoverStackLayout getHomeCoverStackLayout(const GfxRenderer& renderer, Rect rect, int coverCount) const override;
  // The cover band draws the bookshelf + clock module above the nine-grid.
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                       const char* btn4) const override;
  int getMenuRowHeight(const GfxRenderer& renderer) const override;
  int getListRowStep(bool hasSubtitle) const override;
  int getListPageItems(int contentHeight, bool hasSubtitle) const override;
  void drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                const std::function<std::string(int index)>& rowTitle,
                const std::function<std::string(int index)>& rowSubtitle = nullptr,
                const std::function<UIIcon(int index)>& rowIcon = nullptr,
                const std::function<std::string(int index)>& rowValue = nullptr, bool highlightValue = false,
                const std::function<bool(int index)>& rowDimmed = nullptr, bool showSelection = true,
                const std::function<bool(int index)>& rowHeading = nullptr) const override;
};
