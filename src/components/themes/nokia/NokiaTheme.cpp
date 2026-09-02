#include "NokiaTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <string>

#include "CrossPointSettings.h"
#include "RecentBooksStore.h"
#include "components/icons/apps.h"
#include "components/icons/book.h"
#include "components/icons/bookmark.h"
#include "components/icons/folder.h"
#include "components/icons/hotspot.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "components/icons/weread.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/HeaderDateUtils.h"
#include "util/TimeUtils.h"

namespace {

// Big font constants. On non-CJK builds we use the larger Noto Sans sizes for
// the Nokia "bold soft-key" look; CJK builds keep the 12px UI font so the
// built-in CJK fallback still covers Chinese labels (there is no 16px UI CJK
// font, and rendering tofu in the default theme would be a regression).
#ifdef ENABLE_CHINESE_VERSION
constexpr int kTitleFontId = UI_12_FONT_ID;
constexpr int kHeaderTitleFontId = UI_12_FONT_ID;  // largest inline CJK face
constexpr int kGuideFontId = UI_12_FONT_ID;
constexpr int kAuxFontId = UI_12_FONT_ID;
#else
constexpr int kTitleFontId = NOTOSANS_16_FONT_ID;
constexpr int kHeaderTitleFontId = NOTOSANS_18_FONT_ID;
constexpr int kGuideFontId = NOTOSANS_14_FONT_ID;
constexpr int kAuxFontId = NOTOSANS_12_FONT_ID;
#endif

// Bottom-to-top clearance inside each soft-key / list row so the glyphs sit
// nicely centered with plenty of white space (the "big target" feel).
constexpr int kTapPaddingY = 24;
// Uniform button/tile corner radius, aligned with the screen's rounded-corner
// look; every interactive tile and tab uses the same value.
constexpr int kKeyRadius = 26;
constexpr int kRowRadius = 20;

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

// 32px icon bitmaps (1bpp, MSB-first, bit==0 = ink) drawn at `scale` px per
// source pixel. Mirrors GfxRenderer::drawIcon's orientation mapping so the
// Lyra icon set renders exactly as it does in LyraTheme.
void drawNokiaIcon(const GfxRenderer& renderer, const uint8_t* bitmap, int x, int y, int scale, bool inverted) {
  constexpr int kIconSize = 32;
  constexpr int kRowBytes = kIconSize / 8;
  for (int row = 0; row < kIconSize; ++row) {
    for (int col = 0; col < kIconSize; ++col) {
      const uint8_t byte = bitmap[row * kRowBytes + (col >> 3)];
      if (((byte >> (7 - (col & 7))) & 1U) != 0) continue;
      renderer.fillRect(x + (kIconSize - 1 - row) * scale, y + col * scale, scale, scale, !inverted);
    }
  }
}

const uint8_t* nokiaHomeIcon(const UIIcon icon) {
  switch (icon) {
    case UIIcon::Book:
      return BookIcon;
    case UIIcon::Folder:
      return FolderIcon;
    case UIIcon::Recent:
      return RecentIcon;
    case UIIcon::Library:
    case UIIcon::Opds:
      return LibraryIcon;
    case UIIcon::Transfer:
    case UIIcon::Usb:
      return TransferIcon;
    case UIIcon::Settings:
      return Settings2Icon;
    case UIIcon::Apps:
      return AppsIcon;
    case UIIcon::WeRead:
      return WeReadIcon;
    case UIIcon::Bookmark:
      return BookmarkIcon;
    case UIIcon::Wifi:
      return WifiIcon;
    case UIIcon::Hotspot:
      return HotspotIcon;
    default:
      return nullptr;
  }
}

}  // namespace

void NokiaTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                            const char* subtitle) const {
  (void)subtitle;
  const int sidePadding = NokiaMetrics::values.contentSidePadding;
  const bool showBatteryPercentage =
      SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS;
  // Battery keeps a clear safety margin from the physical screen edge.
  const int edgeMargin = 28;
  const int batteryIconX = rect.x + rect.width - edgeMargin - NokiaMetrics::values.batteryWidth;

  if (title == nullptr) {
    // Nokia home status line: current time left, date + battery right.
    // All three elements share the same vertical baseline for clean alignment.
    const int contentH = renderer.getLineHeight(kHeaderTitleFontId);
    const int baseY = rect.y + std::max(0, (rect.height - contentH) / 2);
    const std::string dateText = HeaderDateUtils::getDisplayDateText();
    char timeBuf[16];
    if (TimeUtils::formatCurrentTime(timeBuf, sizeof(timeBuf), SETTINGS.clockFormat == 1)) {
      const int timeW = renderer.getTextWidth(kHeaderTitleFontId, timeBuf, EpdFontFamily::BOLD);
      const int timeX = rect.x + 36;
      const int timeY = baseY;
      renderer.drawText(kHeaderTitleFontId, timeX, timeY, timeBuf, true, EpdFontFamily::BOLD);
    }
    if (!dateText.empty()) {
      // Date uses the same font as the time so they share one baseline.
      const int dateW = renderer.getTextWidth(kHeaderTitleFontId, dateText.c_str(), EpdFontFamily::REGULAR);
      const int dateX = batteryIconX - 10 - dateW;
      renderer.drawText(kHeaderTitleFontId, dateX, baseY, dateText.c_str(), true, EpdFontFamily::REGULAR);
    }
    // The battery outline's internal +6 offset in drawBatteryRight makes its
    // visual body sit below rect.y, so compensate by centering on that
    // actually painted band instead of the passed rect.
    const int batteryVisualH = NokiaMetrics::values.batteryHeight + 6;
    const int batteryY = baseY + (contentH - batteryVisualH) / 2;
    drawBatteryRight(renderer,
                     Rect{batteryIconX, batteryY, NokiaMetrics::values.batteryWidth,
                          NokiaMetrics::values.batteryHeight},
                     false);
    return;
  }

  // Visible back soft-key on every non-home screen. Tapping the header band
  // already maps to Back on touch boards (wasHeaderTapBack); drawing the
  // button makes the affordance explicit for users without edge-swipe back.
  const std::string backLabel = tr(STR_BACK);
  const int backLabelW = renderer.getTextWidth(kHeaderTitleFontId, backLabel.c_str(), EpdFontFamily::BOLD);
  constexpr int kArrowSpan = 22;
  constexpr int kArrowTextGap = 10;
  constexpr int kBackButtonSidePad = 22;
  const int backBtnW = kArrowSpan + kArrowTextGap + backLabelW + kBackButtonSidePad * 2;
  const int backBtnH = std::min(std::max(48, rect.height - 4), 56);
  const int backBtnX = rect.x + sidePadding;
  const int backBtnY = rect.y + (rect.height - backBtnH) / 2;
  // No button border/background: just the text + arrow, enlarged and bold.
  // Crisp drawn left arrow (the subset CJK fonts lack U+2190), bigger than text.
  const int blockW = kArrowSpan + kArrowTextGap + backLabelW;
  const int blockLeft = backBtnX + (backBtnW - blockW) / 2;
  const int arrowCX = blockLeft + kArrowSpan / 2;
  const int arrowCY = backBtnY + backBtnH / 2;
  const int arrowArm = 12;
  renderer.drawLine(arrowCX + arrowArm, arrowCY, arrowCX - arrowArm, arrowCY, 3, true);
  renderer.drawLine(arrowCX - arrowArm, arrowCY, arrowCX - arrowArm + 8, arrowCY - 8, 3, true);
  renderer.drawLine(arrowCX - arrowArm, arrowCY, arrowCX - arrowArm + 8, arrowCY + 8, 3, true);
  const int backTextX = blockLeft + kArrowSpan + kArrowTextGap;
  const int backTextY = backBtnY + (backBtnH - renderer.getLineHeight(kHeaderTitleFontId)) / 2;
  renderer.drawText(kHeaderTitleFontId, backTextX, backTextY, backLabel.c_str(), true, EpdFontFamily::BOLD);

  const int titleX = backBtnX + backBtnW + 12;
  const int titleY = rect.y + (rect.height - renderer.getLineHeight(kHeaderTitleFontId)) / 2;

  int batteryGroupLeftX = batteryIconX;
  if (false) {
    const int maxTextWidth = renderer.getTextWidth(STATUS_NUMERIC_FONT_ID, "100%");
    const int clearW = maxTextWidth + batteryPercentSpacing + NokiaMetrics::values.batteryWidth;
    const int clearH =
        std::max(renderer.getTextHeight(STATUS_NUMERIC_FONT_ID),
                 NokiaMetrics::values.batteryHeight + 8);
    renderer.fillRect(batteryIconX - maxTextWidth - batteryPercentSpacing, rect.y + 12, clearW, clearH, false);
    batteryGroupLeftX = batteryIconX - maxTextWidth - batteryPercentSpacing;
  }

  const int maxTitleWidth = std::max(0, batteryGroupLeftX - 14 - titleX);
  auto headerTitle = renderer.truncatedText(kHeaderTitleFontId, title, maxTitleWidth, EpdFontFamily::BOLD);
  renderer.drawText(kHeaderTitleFontId, titleX, titleY, headerTitle.c_str(), true, EpdFontFamily::BOLD);
  // Same visual-band correction as the home status bar: drawBatteryRight adds
  // +6 to rect.y internally, so center the larger visual band on the title.
  const int titleLineH = renderer.getLineHeight(kHeaderTitleFontId);
  const int titleBaseY = rect.y + (rect.height - titleLineH) / 2;
  const int batteryVisualH = NokiaMetrics::values.batteryHeight + 6;
  drawBatteryRight(renderer,
                   Rect{batteryIconX, titleBaseY + (titleLineH - batteryVisualH) / 2,
                        NokiaMetrics::values.batteryWidth, NokiaMetrics::values.batteryHeight},
                  false);
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

HomeGridLayout NokiaTheme::getHomeGridLayout(const GfxRenderer& renderer, int itemCount) const {
  HomeGridLayout layout;
  if (itemCount <= 0) return layout;

  constexpr int kColumns = 3;
  constexpr int kGap = 16;
  constexpr int kSidePadding = 36;
  constexpr int kTopOffset = 0;

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  // The grid sits below the bookshelf/clock module. The module height is
  // derived from the square grid so the two always add up to the same band.
  const int gridTop = NokiaMetrics::values.homeTopPadding + getHomeModuleHeight(renderer, itemCount) + kTopOffset;
  // Home has no button-hint band: the grid runs down to the 20 px touch
  // safety margin so the removed hints become roomier tiles.
  const int gridBottom = pageHeight - kSidePadding;
  if (gridBottom <= gridTop) return layout;

  const int gridWidth = pageWidth - kSidePadding * 2;
  const int cellWidth = (gridWidth - (kColumns - 1) * kGap) / kColumns;
  if (cellWidth <= 0) return layout;

  const int rows = (itemCount + kColumns - 1) / kColumns;
  const int availableHeight = gridBottom - gridTop;
  // Tiles keep the width of a column but their height adapts to the room left
  // below the cover band, so three rows of menu items never run off-screen.
  // Home tiles stay close to the design's 116 px height even when the grid
  // band is tall; the extra room becomes balanced margins around the dock.
  // No square clamp: tiles fill all available vertical space so the bottom
  // margin equals the side margin. Only enforce a minimum for safety.
  const int cellHeight = std::max(80, (availableHeight - (rows - 1) * kGap) / rows);
  if (cellHeight <= 0) return layout;

  const int gridHeight = rows * cellHeight + (rows - 1) * kGap;
  // Top-align the grid: the gap between the now-reading card and the first
  // dock row must equal the inter-tile gap, so no vertical centering.
  layout.columns = kColumns;
  layout.rows = rows;
  layout.cellX = (pageWidth - gridWidth) / 2;
  layout.cellY = gridTop;
  layout.cellWidth = cellWidth;
  layout.cellHeight = cellHeight;
  layout.gap = kGap;
  return layout;
}

int NokiaTheme::getHomeModuleHeight(const GfxRenderer& renderer, int itemCount) const {
  (void)renderer;
  (void)itemCount;
  // Hero cover band: 今日 title (y=16) + subtitle + card at y=108, height 196.
  // Card bottom = 108 + 196 = 304. The grid's kTopOffset is 0 and the grid
  // is top-aligned, so the module height must end exactly at the card bottom
  // for the gap from card to first dock row to equal the inter-tile kGap (16).
  // Card bottom = 304; add kGap (16) so the first dock row sits one gap below.
  return 304 + 16;
}

void NokiaTheme::drawHomeMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                              const std::function<std::string(int index)>& buttonLabel,
                              const std::function<UIIcon(int index)>& rowIcon) const {
  const HomeGridLayout layout = getHomeGridLayout(renderer, buttonCount);
  if (!layout.isGrid()) {
    // Fallback for degenerate screens: keep the soft-key list look.
    drawButtonMenu(renderer, rect, buttonCount, selectedIndex, buttonLabel, rowIcon);
    return;
  }

  const int lineHeight = renderer.getLineHeight(kTitleFontId);
  // Tall tiles get the 2x icon; short (3-row) grids drop to 1x so the icon
  // and label still fit inside the tile.
  const bool largeIcon = layout.cellHeight >= 88;
  const int kIconSize = largeIcon ? 64 : 32;
  const int kIconScale = largeIcon ? 2 : 1;
  constexpr int kLabelGap = 6;

  for (int i = 0; i < buttonCount; ++i) {
    const int col = i % layout.columns;
    const int row = i / layout.columns;
    const int x = layout.cellX + col * (layout.cellWidth + layout.gap);
    const int y = layout.cellY + row * (layout.cellHeight + layout.gap);
    const bool isSelected = selectedIndex == i;

    // Big rounded soft-key tile. Selected modules invert: black fill with
    // white glyph/label for an unmistakable focus state.
    renderer.fillRoundedRect(x, y, layout.cellWidth, layout.cellHeight, kKeyRadius,
                             isSelected ? Color::Black : Color::White);
    if (!isSelected) renderer.drawRoundedRect(x, y, layout.cellWidth, layout.cellHeight, 1, kKeyRadius, true);

    const uint8_t* iconBitmap = nokiaHomeIcon(rowIcon(i));
    if (iconBitmap != nullptr) {
      const int iconX = x + (layout.cellWidth - kIconSize) / 2;
      const int blockHeight = kIconSize + kLabelGap + lineHeight;
      const int iconY = y + std::max(6, (layout.cellHeight - blockHeight) / 2);
      drawNokiaIcon(renderer, iconBitmap, iconX, iconY, kIconScale, isSelected);
      const std::string label =
          renderer.truncatedText(kTitleFontId, buttonLabel(i).c_str(), std::max(1, layout.cellWidth - 12),
                                 EpdFontFamily::BOLD);
      const int labelW = renderer.getTextWidth(kTitleFontId, label.c_str(), EpdFontFamily::BOLD);
      renderer.drawText(kTitleFontId, x + (layout.cellWidth - labelW) / 2, iconY + kIconSize + kLabelGap,
                        label.c_str(), !isSelected, EpdFontFamily::BOLD);
    } else {
      // No icon: center the label vertically in the tile.
      const std::string label =
          renderer.truncatedText(kTitleFontId, buttonLabel(i).c_str(), std::max(1, layout.cellWidth - 12),
                                 EpdFontFamily::BOLD);
      const int labelW = renderer.getTextWidth(kTitleFontId, label.c_str(), EpdFontFamily::BOLD);
      renderer.drawText(kTitleFontId, x + (layout.cellWidth - labelW) / 2,
                        y + (layout.cellHeight - lineHeight) / 2, label.c_str(), !isSelected,
                        EpdFontFamily::BOLD);
    }
  }
}

void NokiaTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                     int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                     bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  (void)selectorIndex;
  // Keep the snapshot flags false: the module redraws from SD on every full
  // frame, so there is no cover buffer to restore.
  coverRendered = false;
  coverBufferStored = false;
  bufferRestored = false;
  (void)storeCoverBuffer;

  // Home hero: 今日 title, subtitle, and one prominent "reading now" card.
  constexpr int kContentLeft = 36;
  constexpr int kCardWidth = 480;
  constexpr int kCardHeight = 196;
  constexpr int kCoverWidth = 132;
  const int titleLine = renderer.getLineHeight(kHeaderTitleFontId);
  const int auxLine = renderer.getLineHeight(kAuxFontId);
  const int cardX = rect.x + kContentLeft;
  // Card sits at kGap (16px) below the subtitle line, matching the inter-tile
  // gap so the spacing from card-bottom to first dock row also equals kGap.
  const int cardY = rect.y + 108;
  const int cardBottom = cardY + kCardHeight;
  if (cardBottom > rect.y + rect.height) return;

  renderer.drawText(kHeaderTitleFontId, cardX, rect.y + 16, tr(STR_TODAY), true, EpdFontFamily::BOLD);
  renderer.drawText(kAuxFontId, cardX, rect.y + 16 + titleLine + 6, tr(STR_HOME_SUBTITLE), true,
                    EpdFontFamily::REGULAR);

  // Now-reading card: outlined rounded rectangle, pure white surface.
  renderer.fillRoundedRect(cardX, cardY, kCardWidth, kCardHeight, 14, Color::White);
  renderer.drawRoundedRect(cardX, cardY, kCardWidth, kCardHeight, 1, 14, true);
  renderer.drawLine(cardX + kCoverWidth, cardY, cardX + kCoverWidth, cardBottom - 1, 1, true);

  if (recentBooks.empty()) {
    UITheme::drawCenteredWrappedText(renderer, Rect{cardX, cardY, kCardWidth, kCardHeight}, kAuxFontId,
                                     tr(STR_NO_RECENT_BOOKS), 2);
    return;
  }

  const RecentBook& book = recentBooks[0];
  // Cover zone fills the full 132px-wide left column (full card height).
  const Rect coverRect{cardX, cardY, kCoverWidth, kCardHeight};
  bool drewCover = false;
  if (!book.coverBmpPath.empty()) {
    const std::string coverBmpPath =
        UITheme::getCoverThumbPath(book.coverBmpPath, NokiaMetrics::values.homeCoverHeight);
    HalFile file;
    if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        drewCover = renderer.drawBitmapCropToFill(bitmap, coverRect.x, coverRect.y, coverRect.width,
                                                   coverRect.height);
      }
    }
  }
  if (!drewCover) renderer.fillRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, false);
  renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, 14, true);

  const int metaX = cardX + kCoverWidth + 18;
  const int metaWidth = std::max(1, cardX + kCardWidth - 18 - metaX);
  renderer.drawText(kAuxFontId, metaX, cardY + 16, tr(STR_READING_NOW), true, EpdFontFamily::REGULAR);
  const std::string bookTitle =
      renderer.truncatedText(kTitleFontId, book.title.c_str(), metaWidth, EpdFontFamily::BOLD);
  renderer.drawText(kTitleFontId, metaX, cardY + 16 + auxLine + 8, bookTitle.c_str(), true, EpdFontFamily::BOLD);
  if (!book.author.empty()) {
    const std::string author =
        renderer.truncatedText(kAuxFontId, book.author.c_str(), metaWidth, EpdFontFamily::REGULAR);
    renderer.drawText(kAuxFontId, metaX, cardY + 16 + auxLine * 2 + 8 + titleLine, author.c_str(), true,
                      EpdFontFamily::REGULAR);
  }

  // Progress bar + progress text, per the 0902 design:
  // an 8px rounded outlined bar with black fill.
  constexpr int kProgressBarHeight = 8;
  constexpr int kProgressBarRadius = 4;
  const int progressBarY = cardBottom - auxLine - 20 - kProgressBarHeight - 12;
  const int progressBarW = metaWidth;
  // RecentBook has no persisted progress; use a visual placeholder until the
  // store tracks position (the card still matches the design's layout).
  const int progressPercent = 42;
  renderer.drawRoundedRect(metaX, progressBarY, progressBarW, kProgressBarHeight, 1, kProgressBarRadius, true);
  const int progressFillW = ((progressBarW - 4) * progressPercent) / 100;
  if (progressFillW > 0) {
    renderer.fillRoundedRect(metaX + 2, progressBarY + 2, progressFillW, kProgressBarHeight - 4,
                             kProgressBarRadius - 2, Color::Black);
  }

  // Progress text line: left = "继续阅读", right = percentage.
  renderer.drawText(kAuxFontId, metaX, cardBottom - auxLine - 12, tr(STR_RESUME), true, EpdFontFamily::REGULAR);
  char percentText[8];
  snprintf(percentText, sizeof(percentText), "%d%%", progressPercent);
  const int percentTextW = renderer.getTextWidth(kAuxFontId, percentText, EpdFontFamily::REGULAR);
  renderer.drawText(kAuxFontId, metaX + metaWidth - percentTextW, cardBottom - auxLine - 12, percentText, true,
                    EpdFontFamily::REGULAR);
}

HomeCoverStackLayout NokiaTheme::getHomeCoverStackLayout(const GfxRenderer& renderer, Rect rect,
                                                          int coverCount) const {
  HomeCoverStackLayout layout;
  if (coverCount <= 0 || rect.width <= 0 || rect.height <= 0) return layout;

  // Single tappable hero card: the most recent book.
  layout.x = rect.x + 36;
  layout.y = rect.y + 108;
  layout.width = 480;
  layout.height = 196;
  layout.count = 1;
  return layout;
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
  const int bottomMargin = 20;
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
