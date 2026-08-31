#include "MovieActivity.h"

#include <Arduino.h>
#include <I18n.h>
#include <esp_system.h>

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
    {u8"千与千寻", u8"2001", u8"动画/奇幻", u8"少女千寻误入神灵世界，为救父母走上一段勇敢的成长之旅。"},
    {u8"肖申克的救赎", u8"1994", u8"剧情", u8"银行家蒙冤入狱，用二十年与智慧凿出通往自由的路。"},
    {u8"盗梦空间", u8"2010", u8"科幻/悬疑", u8"梦境中的层层嵌套，一场关于现实边界的华丽冒险。"},
    {u8"海上钢琴师", u8"1998", u8"剧情/音乐", u8"一生未曾下船的天才琴师，用琴声丈量无边的大海。"},
    {u8"霸王别姬", u8"1993", u8"剧情/爱情", u8"两位京剧伶人半个世纪的悲欢离合与时代浮沉。"},
    {u8"当幸福来敲门", u8"2006", u8"剧情/传记", u8"落魄父亲携子逐梦，绝境中守住尊严与希望。"},
    {u8"星际穿越", u8"2014", u8"科幻/冒险", u8"穿越虫洞寻找新家园，爱与引力跨越维度。"},
    {u8"寻梦环游记", u8"2017", u8"动画/家庭", u8"亡灵节上，男孩在音乐与记忆中重拾亲情。"},
    {u8"怦然心动", u8"2010", u8"爱情/剧情", u8"两小无猜的初恋视角，梧桐树下的纯真告白。"},
    {u8"疯狂动物城", u8"2016", u8"动画/喜剧", u8"兔子警官与狐狸搭档，揭开城市背后的温情谜案。"},
    {u8"大话西游", u8"1995", u8"喜剧/奇幻", u8"至尊宝的宿命轮回，一句台词道尽意难平。"},
    {u8"三傻大闹宝莱坞", u8"2009", u8"剧情/喜剧", u8"以乐观与热爱反抗僵化教育，追求真正的自我。"},
};
constexpr int kMovieCount = static_cast<int>(sizeof(kMovies) / sizeof(kMovies[0]));

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