#pragma once

#include "CrossPointSettings.h"
#include "components/UITheme.h"
#include "fontIds.h"

// FreeInkUI font slots. Row heights, header height, and touch sizes are not
// chosen here: FreeInkApp derives its default metric tokens from the body
// font's line height. CrossMux list screens historically use UI_10; titles
// remain UI_12.
struct UIScaleSpec {
  int smallFontId;
  int bodyFontId;
  int titleFontId;
};

inline UIScaleSpec uiScaleSpec() {
  UIScaleSpec spec{};
  // The Nokia theme is built around large soft-key text, so its FreeInkUI
  // screens (settings, file browser, recents, apps) use the same body size as
  // the home grid instead of the tiny 10px default. CJK builds keep UI_12 (the
  // largest inline CJK face); other builds use Noto Sans 16 like the home.
#ifdef ENABLE_CHINESE_VERSION
  constexpr int kLargeBodyFont = UI_12_FONT_ID;
  constexpr int kLargeTitleFont = UI_12_FONT_ID;
  constexpr int kLargeSmallFont = UI_10_FONT_ID;
#else
  constexpr int kLargeBodyFont = NOTOSANS_16_FONT_ID;
  constexpr int kLargeTitleFont = NOTOSANS_16_FONT_ID;
  constexpr int kLargeSmallFont = NOTOSANS_14_FONT_ID;
#endif
  const bool nokiaTheme =
      UITheme::getInstance().getType() == CrossPointSettings::UI_THEME::NOKIA;
  spec.smallFontId = nokiaTheme ? kLargeSmallFont : UI_10_FONT_ID;
  spec.bodyFontId = nokiaTheme ? kLargeBodyFont : UI_10_FONT_ID;
  // Titles use the UI font, not a reader font: fui headers draw book and
  // directory titles, and the built-in Ubuntu UI fonts cover Hebrew (plus the
  // size-matched SD CJK fallback) where the NotoSans reader subsets do not.
  // Same font develop's drawHeader used, so script coverage matches develop.
  spec.titleFontId = nokiaTheme ? kLargeTitleFont : UI_12_FONT_ID;
  return spec;
}
