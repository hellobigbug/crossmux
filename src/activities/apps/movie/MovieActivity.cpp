#include "MovieActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <esp_system.h>
#include <math.h>

#include "components/UITheme.h"
#include "fontIds.h"

namespace {

struct Movie {
  const char* title;
  const char* year;
  const char* genre;
  const char* synopsis;
};

constexpr Movie kMovies[] = {
    {"千与千寻", "2001", "动画/奇幻", "少女千寻误入神灵世界，为救父母走上一段勇敢的成长之旅。"},
    {"肖申克的救赎", "1994", "剧情", "银行家蒙冤入狱，用二十年与智慧凿出通往自由的路。"},
    {"盗梦空间", "2010", "科幻/悬疑", "梦境中的层层嵌套，一场关于现实边界的华丽冒险。"},
    {"海上钢琴师", "1998", "剧情/音乐", "一生未曾下船的天才琴师，用琴声丈量无边的大海。"},
    {"霸王别姬", "1993", "剧情/爱情", "两位京剧伶人半个世纪的悲欢离合与时代浮沉。"},
    {"当幸福来敲门", "2006", "剧情/传记", "落魄父亲携子逐梦，绝境中守住尊严与希望。"},
    {"星际穿越", "2014", "科幻/冒险", "穿越虫洞寻找新家园，爱与引力跨越维度。"},
    {"寻梦环游记", "2017", "动画/家庭", "亡灵节上，男孩在音乐与记忆中重拾亲情。"},
    {"怦然心动", "2010", "爱情/剧情", "两小无猜的初恋视角，梧桐树下的纯真告白。"},
    {"疯狂动物城", "2016", "动画/喜剧", "兔子警官与狐狸搭档，揭开城市背后的温情谜案。"},
    {"大话西游", "1995", "喜剧/奇幻", "至尊宝的宿命轮回，一句台词道尽意难平。"},
    {"三傻大闹宝莱坞", "2009", "剧情/喜剧", "以乐观与热爱反抗僵化教育，追求真正的自我。"},
};
constexpr int kMovieCount = static_cast<int>(sizeof(kMovies) / sizeof(kMovies[0]));

// Draw a filled 5-pointed star centred at (cx, cy) with the given outer radius.
// Rotation is fixed; used purely for the rating row.
void drawStar(GfxRenderer& renderer, const int cx, const int cy, const int radius, const bool filled) {
  int xPts[10];
  int yPts[10];
  const int inner = radius * 2 / 5;
  for (int i = 0; i < 10; ++i) {
    const float ang = (i * 36 - 90) * 3.14159265f / 180.0f;
    const int r = (i % 2 == 0) ? radius : inner;
    xPts[i] = cx + static_cast<int>(r * cosf(ang));
    yPts[i] = cy + static_cast<int>(r * sinf(ang));
  }
  if (filled) {
    renderer.fillPolygon(xPts, yPts, 10, true);
  } else {
    for (int i = 0; i < 10; ++i) {
      renderer.drawLine(xPts[i], yPts[i], xPts[(i + 1) % 10], yPts[(i + 1) % 10], 1, true);
    }
  }
}

}  // namespace

void MovieActivity::onEnter() {
  Activity::onEnter();
  pick();
}

void MovieActivity::pick() {
  index_ = static_cast<int>(esp_random() % kMovieCount);
  requestUpdate();
}

void MovieActivity::loop() {
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

void MovieActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_MOVIE_TITLE));

  const Movie& m = kMovies[index_];
  const int contentLeft = metrics.contentSidePadding;
  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 30;

  // Poster placeholder: a framed rect with a play triangle.
  const int posterW = 110;
  const int posterH = 160;
  renderer.drawRect(contentLeft, contentTop, posterW, posterH, 2, true);
  const int tx = contentLeft + 24;
  const int ty = contentTop + posterH / 2;
  renderer.fillRect(tx, ty - 16, 8, 32, true);
  renderer.drawLine(tx + 8, ty - 16, tx + 26, ty, 2, true);
  renderer.drawLine(tx + 8, ty + 16, tx + 26, ty, 2, true);

  // Title block to the right of the poster.
  const int titleX = contentLeft + posterW + metrics.verticalSpacing;
  const int titleW = pageWidth - titleX - contentLeft;
  const auto titleLines = renderer.wrappedText(UI_12_FONT_ID, m.title, titleW, 2, EpdFontFamily::BOLD);
  int y = contentTop;
  for (const auto& line : titleLines) {
    renderer.drawText(UI_12_FONT_ID, titleX, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(UI_12_FONT_ID);
  }
  y += 4;
  char meta[32];
  snprintf(meta, sizeof(meta), "%s  ·  %s", m.year, m.genre);
  renderer.drawText(SMALL_FONT_ID, titleX, y, meta, true, EpdFontFamily::REGULAR);

  // Star rating row (5 stars). Deterministic per film; filled stars earned.
  const int rating = 3 + (static_cast<int>(index_ * 7) % 3);
  const int starY = y + renderer.getLineHeight(SMALL_FONT_ID) + 6;
  const int starR = 6;
  const int starGap = starR * 2 + 4;
  for (int s = 0; s < 5; ++s) {
    drawStar(renderer, titleX + starR + s * starGap, starY, starR, s < rating);
  }

  // Synopsis card below.
  const int boxTop = contentTop + posterH + metrics.verticalSpacing;
  const int boxW = pageWidth - 2 * contentLeft;
  const int boxH = pageHeight - boxTop - metrics.buttonHintsHeight - metrics.verticalSpacing - 24;
  if (boxH > 0) {
    renderer.fillRoundedRect(contentLeft, boxTop, boxW, boxH, metrics.controlRadius, Color::White);
    renderer.drawRoundedRect(contentLeft, boxTop, boxW, boxH, 2, metrics.controlRadius, true);
    const int wrapW = boxW - 2 * metrics.verticalSpacing;
    const int maxLines = (boxH - 2 * metrics.verticalSpacing) / renderer.getLineHeight(UI_12_FONT_ID);
    const auto lines = renderer.wrappedText(UI_12_FONT_ID, m.synopsis, wrapW, maxLines, EpdFontFamily::REGULAR);
    int sy = boxTop + metrics.verticalSpacing;
    for (const auto& line : lines) {
      renderer.drawText(UI_12_FONT_ID, contentLeft + metrics.verticalSpacing, sy, line.c_str(), true,
                        EpdFontFamily::REGULAR);
      sy += renderer.getLineHeight(UI_12_FONT_ID);
    }
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_MOVIE_NEXT), "", tr(STR_MOVIE_NEXT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}