#include "HomeActivity.h"

#include <Bitmap.h>
#include <Epub.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Memory.h>
#include <Utf8.h>
#include <Xtc.h>

#include <algorithm>
#include <cstring>
#include <vector>

#include "CrossPointSettings.h"
#include "CrossPointState.h"
#include "MappedInputManager.h"
#include "OpdsServerStore.h"
#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/BookCoverLoader.h"

namespace {
struct HomeMenuEntry {
  HomeMenuItem item;
  StrId label;
  UIIcon icon;
};

constexpr HomeMenuEntry kDefaultMenuOrder[] = {
    {HomeMenuItem::RECENTS, StrId::STR_MENU_RECENT_BOOKS, Recent},
    {HomeMenuItem::FILE_BROWSER, StrId::STR_BROWSE_FILES, Folder},
    {HomeMenuItem::FILE_TRANSFER, StrId::STR_FILE_TRANSFER, Transfer},
#ifdef ENABLE_CHINESE_VERSION
    {HomeMenuItem::WEREAD, StrId::STR_WEREAD_TITLE, UIIcon::WeRead},
#endif
    {HomeMenuItem::APPS, StrId::STR_APPS_TITLE, Apps},
    {HomeMenuItem::SETTINGS_MENU, StrId::STR_SETTINGS_TITLE, Settings},
};
constexpr HomeMenuEntry kCarouselMenuOrder[] = {
    {HomeMenuItem::FILE_BROWSER, StrId::STR_BROWSE_FILES, Folder},
    {HomeMenuItem::RECENTS, StrId::STR_MENU_RECENT_BOOKS, Recent},
    {HomeMenuItem::APPS, StrId::STR_APPS_TITLE, Apps},
    {HomeMenuItem::FILE_TRANSFER, StrId::STR_FILE_TRANSFER, Transfer},
    {HomeMenuItem::SETTINGS_MENU, StrId::STR_SETTINGS_TITLE, Settings},
#ifdef ENABLE_CHINESE_VERSION
    {HomeMenuItem::WEREAD, StrId::STR_WEREAD_TITLE, UIIcon::WeRead},
#endif
};
#ifdef ENABLE_CHINESE_VERSION
constexpr int kHomeMenuItemCount = 6;
#else
constexpr int kHomeMenuItemCount = 5;
#endif

constexpr const HomeMenuEntry* menuEntryAtIndex(int index, bool hasOpds, bool carousel) {
  (void)hasOpds;
  if (index < 0) return nullptr;
  if (index >= kHomeMenuItemCount) return nullptr;
  return &(carousel ? kCarouselMenuOrder : kDefaultMenuOrder)[index];
}

constexpr HomeMenuItem indexToMenuItem(int index, bool hasOpds, bool carousel) {
  const HomeMenuEntry* entry = menuEntryAtIndex(index, hasOpds, carousel);
  return entry == nullptr ? HomeMenuItem::NONE : entry->item;
}

constexpr int menuItemToIndex(HomeMenuItem item, bool hasOpds, bool carousel) {
  (void)hasOpds;
  const int count = kHomeMenuItemCount;
  for (int i = 0; i < count; ++i) {
    if (indexToMenuItem(i, hasOpds, carousel) == item) return i;
  }
  return 0;
}

static_assert(indexToMenuItem(2, true, true) == HomeMenuItem::APPS);
static_assert(indexToMenuItem(4, true, true) == HomeMenuItem::SETTINGS_MENU);
static_assert(indexToMenuItem(2, true, false) == HomeMenuItem::FILE_TRANSFER);
#ifdef ENABLE_CHINESE_VERSION
static_assert(indexToMenuItem(4, true, false) == HomeMenuItem::APPS);
#else
static_assert(indexToMenuItem(3, true, false) == HomeMenuItem::APPS);
#endif
static_assert(menuItemToIndex(HomeMenuItem::APPS, true, true) == 2);
static_assert(menuItemToIndex(HomeMenuItem::SETTINGS_MENU, true, true) == 4);
#ifdef ENABLE_CHINESE_VERSION
static_assert(menuItemToIndex(HomeMenuItem::WEREAD, true, true) == 5);
#endif

// Move a selection inside a row-major home grid, wrapping at the grid edges.
// A wrap target past the end of a short row clamps to the last item.
int gridMoveIndex(int current, int itemCount, const HomeGridLayout& layout, int dCol, int dRow) {
  if (itemCount <= 0 || current < 0 || current >= itemCount || !layout.isGrid()) return current;
  const int columns = layout.columns;
  const int rows = layout.rows;
  int row = current / columns;
  int col = current % columns;
  row = (row + dRow + rows) % rows;
  col = (col + dCol + columns) % columns;
  const int next = row * columns + col;
  return next < itemCount ? next : itemCount - 1;
}
}  // namespace

int HomeActivity::getMenuItemCount() const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  if (usesGridHome()) {
    // Grid homes keep books out of the menu entirely: covers live in the
    // stacked cascade band above the grid.
    return kHomeMenuItemCount;
  }
  int count = kHomeMenuItemCount;
  if (metrics.homeContinueReadingInMenu) {
    // Continue-reading is a single row in the menu, however many covers the
    // bookshelf module shows.
    if (!recentBooks.empty()) ++count;
  } else {
    count += static_cast<int>(recentBooks.size());
  }
  return count;
}

bool HomeActivity::usesGridHome() const {
  return UITheme::getInstance().getType() == CrossPointSettings::UI_THEME::NOKIA;
}

void HomeActivity::requestCarouselUpdate(CarouselUpdateScope scope) {
  if (scope == CarouselUpdateScope::Full) {
    carouselUpdateScope = scope;
  } else {
    // A queued full redraw must not be downgraded by a later menu event.
    CarouselUpdateScope expected = CarouselUpdateScope::None;
    carouselUpdateScope.compare_exchange_strong(expected, CarouselUpdateScope::MenuOnly);
  }
  requestUpdate();
}

void HomeActivity::loadRecentBooks(int maxBooks) {
  recentBooks.clear();
  const auto& books = RECENT_BOOKS.getBooks();
  recentBooks.reserve(std::min(static_cast<int>(books.size()), maxBooks));

  for (const RecentBook& book : books) {
    // Limit to maximum number of recent books
    if (recentBooks.size() >= maxBooks) {
      break;
    }

    // Skip if file no longer exists
    if (RecentBooksStore::isMissing(book)) {
      continue;
    }

    recentBooks.push_back(book);
  }
}

void HomeActivity::loadRecentCovers(int coverHeight) {
  recentsLoading = true;
  bool showingLoading = false;
  Rect popupRect;
  const bool useFullCover =
      static_cast<CrossPointSettings::UI_THEME>(SETTINGS.uiTheme) == CrossPointSettings::UI_THEME::LYRA_CAROUSEL;

  int progress = 0;
  for (RecentBook& book : recentBooks) {
    const int currentProgress = progress++;
    const bool isEpub = FsHelpers::hasEpubExtension(book.path);
    const bool isXtc = FsHelpers::hasXtcExtension(book.path);

    // Keep the persisted path theme-neutral; Carousel redirects only this activity's copy.
    if (isEpub) {
      const Epub epub(book.path, "/.crosspoint");
      if (useFullCover) {
        const std::string thumbPath = epub.getThumbBmpPath();
        if (book.coverBmpPath != thumbPath) {
          RECENT_BOOKS.updateBook(book.path, book.title, book.author, thumbPath);
        }
        book.coverBmpPath = epub.getCoverBmpPath();
      } else if (book.coverBmpPath.empty()) {
        book.coverBmpPath = epub.getThumbBmpPath();
        RECENT_BOOKS.updateBook(book.path, book.title, book.author, book.coverBmpPath);
      }
    } else if (isXtc) {
      const Xtc xtc(book.path, "/.crosspoint");
      if (useFullCover) {
        const std::string thumbPath = xtc.getThumbBmpPath();
        if (book.coverBmpPath != thumbPath) {
          RECENT_BOOKS.updateBook(book.path, book.title, book.author, thumbPath);
        }
        book.coverBmpPath = xtc.getCoverBmpPath();
      } else if (book.coverBmpPath.empty()) {
        book.coverBmpPath = xtc.getThumbBmpPath();
        RECENT_BOOKS.updateBook(book.path, book.title, book.author, book.coverBmpPath);
      }
    } else {
      continue;
    }
    if (book.coverBmpPath.empty()) continue;

    const std::string coverPath = UITheme::getCoverThumbPath(book.coverBmpPath, coverHeight);
    if (Storage.exists(coverPath.c_str())) {
      bool invalidCache = false;
      {
        HalFile cachedCover;
        if (!Storage.openFileForRead("HOME", coverPath, cachedCover)) {
          LOG_ERR("HOME", "Failed to open cached cover: %s", coverPath.c_str());
          continue;
        }
        if (cachedCover.fileSize() == 0) {
          // EPUB uses an empty thumbnail as a persistent "no supported cover" marker.
          if (isEpub && !useFullCover) continue;
          invalidCache = true;
        } else {
          Bitmap bitmap(cachedCover);
          invalidCache =
              bitmap.parseHeaders() != BmpReaderError::Ok || bitmap.getWidth() <= 0 || bitmap.getHeight() <= 0;
        }
      }
      if (!invalidCache) continue;
      LOG_ERR("HOME", "Removing invalid cached cover: %s", coverPath.c_str());
      if (!Storage.remove(coverPath.c_str())) {
        LOG_ERR("HOME", "Failed to remove invalid cached cover: %s", coverPath.c_str());
        continue;
      }
    }

    if (!showingLoading) {
      showingLoading = true;
      popupRect = GUI.drawPopup(renderer, tr(STR_LOADING_POPUP));
    }
    GUI.fillPopupProgress(renderer, popupRect, 10 + currentProgress * (90 / recentBooks.size()));
    std::string generatedPath;
    if (isEpub) {
      {
        GfxRenderer::FrameBufferLoan loan(renderer);
        generatedPath = useFullCover ? BookCoverLoader::ensureFullCover(book.path)
                                     : BookCoverLoader::ensureThumbnail(book.path, coverHeight);
      }
      // Inflate used the old framebuffer bytes. Rebuild a complete, known
      // loading frame before the next progress refresh can reach the panel.
      renderer.clearScreen();
      popupRect = GUI.drawPopup(renderer, tr(STR_LOADING_POPUP));
      GUI.fillPopupProgress(renderer, popupRect, 10 + currentProgress * (90 / recentBooks.size()));
    } else {
      generatedPath = useFullCover ? BookCoverLoader::ensureFullCover(book.path)
                                   : BookCoverLoader::ensureThumbnail(book.path, coverHeight);
    }
    if (generatedPath.empty() && isXtc) LOG_ERR("HOME", "Failed to generate XTC cover: %s", book.path.c_str());
  }

  recentsLoaded = true;
  recentsLoading = false;
  coverRendered = false;
  coverBufferStored = false;
  requestCarouselUpdate(CarouselUpdateScope::Full);
}

void HomeActivity::onEnter() {
  Activity::onEnter();

  hasOpdsServers = OPDS_STORE.hasServers();

  const auto& metrics = UITheme::getInstance().getMetrics();
  loadRecentBooks(metrics.homeRecentBooksCount);

  const bool gridHome = usesGridHome();
  const bool continueReadingInMenu = metrics.homeContinueReadingInMenu && !gridHome;
  const auto base =
      gridHome ? 0 : (continueReadingInMenu ? (recentBooks.empty() ? 0 : 1)
                                            : static_cast<int>(recentBooks.size()));
  const bool isCarousel =
      static_cast<CrossPointSettings::UI_THEME>(SETTINGS.uiTheme) == CrossPointSettings::UI_THEME::LYRA_CAROUSEL;
  selectorIndex =
      initialMenuItem == HomeMenuItem::NONE ? 0 : base + menuItemToIndex(initialMenuItem, hasOpdsServers, isCarousel);
  lastCarouselBookIndex = 0;

  // Trigger first update
  requestUpdate();
}

void HomeActivity::onExit() {
  Activity::onExit();

  // Free the stored cover buffer if any
  freeCoverBuffer();
}

bool HomeActivity::storeCoverBuffer() {
  if (coverBufferUnavailable) return false;

  // Thumbnail generation may borrow the framebuffer; cache only the final render.
  if (!recentsLoaded) return false;

  // render() must have already set the cover rect; without it we'd be back to
  // cloning the whole framebuffer.
  if (coverRectW <= 0 || coverRectH <= 0) return false;
  const size_t needed = renderer.getRegionByteSize(coverRectX, coverRectY, coverRectW, coverRectH);
  if (needed == 0) return false;

  if (!coverBuffer || coverBufferSize < needed) {
    // The carousel region is up to ~44 KB, too large for the task stack. Allocate
    // once and reuse it for every selection during this HomeActivity lifetime.
    auto replacement = makeUniqueNoThrow<uint8_t[]>(needed);
    if (!replacement) {
      LOG_ERR("HOME", "OOM: cover buffer (%u bytes)", (unsigned)needed);
      // ponytail: the theme/region is fixed for this Activity lifetime; retry
      // only after re-entering Home, when heap fragmentation may have changed.
      coverBufferUnavailable = true;
      return false;
    }
    coverBuffer = std::move(replacement);
    coverBufferSize = needed;
  }

  if (!renderer.copyRegionToBuffer(coverRectX, coverRectY, coverRectW, coverRectH, coverBuffer.get(),
                                   coverBufferSize)) {
    return false;
  }
  return true;
}

bool HomeActivity::restoreCoverBuffer() {
  if (!coverBuffer || coverRectW <= 0 || coverRectH <= 0) return false;
  return renderer.copyBufferToRegion(coverRectX, coverRectY, coverRectW, coverRectH, coverBuffer.get(),
                                     coverBufferSize);
}

void HomeActivity::freeCoverBuffer() {
  coverBuffer.reset();
  coverBufferSize = 0;
  coverBufferStored = false;
  coverBufferUnavailable = false;
}

void HomeActivity::loop() {
  const int menuCount = getMenuItemCount();
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int bookCount = static_cast<int>(recentBooks.size());
  const bool isCarousel =
      static_cast<CrossPointSettings::UI_THEME>(SETTINGS.uiTheme) == CrossPointSettings::UI_THEME::LYRA_CAROUSEL;
  const bool gridHome = usesGridHome();
  const bool continueReadingInMenu = metrics.homeContinueReadingInMenu && !gridHome;
  const int renderedMenuCount = gridHome ? menuCount : menuCount - (continueReadingInMenu ? 0 : bookCount);
  const HomeGridLayout homeGrid = GUI.getHomeGridLayout(renderer, renderedMenuCount);
  // Grid cells are indexed in menu space (renderedMenuCount items); the
  // selectorIndex is offset by the book count unless Continue-reading rows are
  // part of the menu itself.
  const int gridSelectionOffset = gridHome ? 0 : (continueReadingInMenu ? 0 : bookCount);

  const auto moveGridSelection = [this, &homeGrid, renderedMenuCount, gridSelectionOffset](int dCol, int dRow) {
    const int gridSelected = selectorIndex - gridSelectionOffset;
    const int next = gridMoveIndex(gridSelected, renderedMenuCount, homeGrid, dCol, dRow);
    selectorIndex = next + gridSelectionOffset;
    requestUpdate();
  };

  auto activateSelection = [this, isCarousel, continueReadingInMenu, gridHome] {
    const int recentCount = static_cast<int>(recentBooks.size());
    const bool bookSelection =
        !gridHome && (continueReadingInMenu ? (recentCount > 0 && selectorIndex == 0) : (selectorIndex < recentCount));
    if (bookSelection) {
      onSelectBook(recentBooks[continueReadingInMenu ? 0 : selectorIndex].path);
      return;
    }
    const int menuIndex = gridHome ? selectorIndex : selectorIndex - (continueReadingInMenu ? 1 : recentCount);
    switch (indexToMenuItem(menuIndex, hasOpdsServers, isCarousel)) {
      case HomeMenuItem::FILE_BROWSER:
        onFileBrowserOpen();
        break;
      case HomeMenuItem::RECENTS:
        onRecentsOpen();
        break;
      case HomeMenuItem::OPDS_BROWSER:
        onOpdsBrowserOpen();
        break;
      case HomeMenuItem::FILE_TRANSFER:
        onFileTransferOpen();
        break;
      case HomeMenuItem::SETTINGS_MENU:
        onSettingsOpen();
        break;
      case HomeMenuItem::APPS:
        onAppsOpen();
        break;
      case HomeMenuItem::WEREAD:
        onWeReadOpen();
        break;
      default:
        break;
    }
  };

  if (isCarousel) {
    const bool coversFocused = selectorIndex < bookCount;
    const int rowIndex = coversFocused ? selectorIndex : selectorIndex - bookCount;

    if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
      if (coversFocused) {
        selectorIndex = ButtonNavigator::nextIndex(rowIndex, bookCount);
        lastCarouselBookIndex = selectorIndex;
      } else {
        selectorIndex = bookCount + ButtonNavigator::nextIndex(rowIndex, renderedMenuCount);
      }
      requestCarouselUpdate(coversFocused ? CarouselUpdateScope::Full : CarouselUpdateScope::MenuOnly);
      return;
    }
    if (mappedInput.wasPressed(MappedInputManager::Button::Left)) {
      if (coversFocused) {
        selectorIndex = ButtonNavigator::previousIndex(rowIndex, bookCount);
        lastCarouselBookIndex = selectorIndex;
      } else {
        selectorIndex = bookCount + ButtonNavigator::previousIndex(rowIndex, renderedMenuCount);
      }
      requestCarouselUpdate(coversFocused ? CarouselUpdateScope::Full : CarouselUpdateScope::MenuOnly);
      return;
    }
    if (mappedInput.wasPressed(MappedInputManager::Button::Up) ||
        mappedInput.wasPressed(MappedInputManager::Button::Down)) {
      if (bookCount > 0) {
        if (coversFocused) {
          lastCarouselBookIndex = selectorIndex;
          selectorIndex = bookCount;
        } else {
          selectorIndex = std::clamp(lastCarouselBookIndex, 0, bookCount - 1);
        }
        requestCarouselUpdate(CarouselUpdateScope::Full);
      }
      return;
    }
  } else if (gridHome) {
    // Retro phone grid: Left/Right walk a row, Up/Down walk a column.
    if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
      moveGridSelection(+1, 0);
      return;
    }
    if (mappedInput.wasPressed(MappedInputManager::Button::Left)) {
      moveGridSelection(-1, 0);
      return;
    }
    if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
      moveGridSelection(0, +1);
      return;
    }
    if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
      moveGridSelection(0, -1);
      return;
    }
  } else {
    buttonNavigator.onNext([this, menuCount] {
      selectorIndex = ButtonNavigator::nextIndex(selectorIndex, menuCount);
      requestUpdate();
    });

    buttonNavigator.onPrevious([this, menuCount] {
      selectorIndex = ButtonNavigator::previousIndex(selectorIndex, menuCount);
      requestUpdate();
    });
  }

  const auto swipe = mappedInput.wasSwipe();
  if (isCarousel) {
    const bool coversFocused = selectorIndex < bookCount;
    const int rowIndex = coversFocused ? selectorIndex : selectorIndex - bookCount;
    switch (swipe) {
      case MappedInputManager::SwipeDir::Left:
        if (coversFocused) {
          selectorIndex = ButtonNavigator::nextIndex(rowIndex, bookCount);
          lastCarouselBookIndex = selectorIndex;
        } else {
          selectorIndex = bookCount + ButtonNavigator::nextIndex(rowIndex, renderedMenuCount);
        }
        requestCarouselUpdate(coversFocused ? CarouselUpdateScope::Full : CarouselUpdateScope::MenuOnly);
        return;
      case MappedInputManager::SwipeDir::Right:
        if (coversFocused) {
          selectorIndex = ButtonNavigator::previousIndex(rowIndex, bookCount);
          lastCarouselBookIndex = selectorIndex;
        } else {
          selectorIndex = bookCount + ButtonNavigator::previousIndex(rowIndex, renderedMenuCount);
        }
        requestCarouselUpdate(coversFocused ? CarouselUpdateScope::Full : CarouselUpdateScope::MenuOnly);
        return;
      case MappedInputManager::SwipeDir::Up:
      case MappedInputManager::SwipeDir::Down:
        if (bookCount > 0) {
          if (coversFocused) {
            lastCarouselBookIndex = selectorIndex;
            selectorIndex = bookCount;
          } else {
            selectorIndex = std::clamp(lastCarouselBookIndex, 0, bookCount - 1);
          }
          requestCarouselUpdate(CarouselUpdateScope::Full);
        }
        return;
      case MappedInputManager::SwipeDir::None:
        break;
    }
  } else if (gridHome) {
    // Horizontal swipes are global navigation on the grid home:
    // Left → Library (recent books), Right → Reading Stats.
   if (swipe == MappedInputManager::SwipeDir::Left) {
      activityManager.goToRecentBooks();
      return;
   }
    if (swipe == MappedInputManager::SwipeDir::Right) {
      activityManager.goToReadingStats();
      return;
    }
    // Vertical swipes still walk the dock grid.
    if (swipe == MappedInputManager::SwipeDir::Up) {
      moveGridSelection(0, +1);
      return;
    }
    if (swipe == MappedInputManager::SwipeDir::Down) {
      moveGridSelection(0, -1);
      return;
    }
  } else {
    if (swipe == MappedInputManager::SwipeDir::Up) {
      selectorIndex = ButtonNavigator::nextIndex(selectorIndex, menuCount);
      requestUpdate();
      return;
    }
    if (swipe == MappedInputManager::SwipeDir::Down) {
      selectorIndex = ButtonNavigator::previousIndex(selectorIndex, menuCount);
      requestUpdate();
      return;
    }
  }

  int tx = 0;
  int ty = 0;
  const bool hasCoverArea = metrics.homeCoverTileHeight > 0;
  if (!gridHome && hasCoverArea && !recentBooks.empty() && mappedInput.wasScreenTouchDown(tx, ty) &&
      tx >= 0 && tx < renderer.getScreenWidth() && ty >= metrics.homeTopPadding &&
      ty < metrics.homeTopPadding + metrics.homeCoverTileHeight) {
    int touchedBook = 0;
    if (isCarousel) {
      const int centerBook =
          selectorIndex < bookCount ? selectorIndex : std::clamp(lastCarouselBookIndex, 0, bookCount - 1);
      if (tx < renderer.getScreenWidth() / 3) {
        touchedBook = ButtonNavigator::previousIndex(centerBook, bookCount);
      } else if (tx >= renderer.getScreenWidth() * 2 / 3) {
        touchedBook = ButtonNavigator::nextIndex(centerBook, bookCount);
      } else {
        touchedBook = centerBook;
      }
      lastCarouselBookIndex = touchedBook;
    }
    if (selectorIndex != touchedBook) {
      selectorIndex = touchedBook;
      requestCarouselUpdate(CarouselUpdateScope::Full);
    }
    return;
  }

  if (!gridHome && hasCoverArea && !recentBooks.empty() &&
      mappedInput.wasTapInRect(0, metrics.homeTopPadding, renderer.getScreenWidth(), metrics.homeCoverTileHeight)) {
    if (!isCarousel) selectorIndex = 0;
    activateSelection();
    return;
  }

  if (gridHome) {
    // Stacked cover cascade: same geometry the theme draws, so taps land on
    // the exact cover the user sees (newest on top, older ones below/right).
    const int coverMenuCount = kHomeMenuItemCount;
    const int coverModuleHeight = GUI.getHomeModuleHeight(renderer, coverMenuCount);
    const Rect coverStackRect{0, metrics.homeTopPadding, renderer.getScreenWidth(), coverModuleHeight};
    const HomeCoverStackLayout coverStack =
        GUI.getHomeCoverStackLayout(renderer, coverStackRect, static_cast<int>(recentBooks.size()));
    if (coverStack.isValid()) {
      if (mappedInput.wasScreenTouchDown(tx, ty)) {
        const int touched = coverStack.indexAt(tx, ty);
        if (touched >= 0) return;
      }
      if (mappedInput.wasScreenTapped(tx, ty)) {
        const int touched = coverStack.indexAt(tx, ty);
        if (touched >= 0) {
          onSelectBook(recentBooks[touched].path);
          return;
        }
      }
    }
    // Hit-test the painted grid cells (same geometry the theme draws with).
    if (mappedInput.wasScreenTouchDown(tx, ty)) {
      const int touched = homeGrid.indexFromPoint(tx, ty, renderedMenuCount);
      if (touched >= 0) {
        const int target = touched + gridSelectionOffset;
        if (selectorIndex != target) {
          selectorIndex = target;
          requestUpdate();
        }
      }
      return;
    }
    if (mappedInput.wasScreenTapped(tx, ty)) {
      const int touched = homeGrid.indexFromPoint(tx, ty, renderedMenuCount);
      if (touched >= 0) {
        selectorIndex = touched + gridSelectionOffset;
        activateSelection();
      }
      return;
    }
  } else {
    const int menuTop = metrics.homeTopPadding + metrics.homeCoverTileHeight + metrics.homeMenuTopOffset;
    const int renderedMenuSelection =
        metrics.homeContinueReadingInMenu ? selectorIndex : selectorIndex - recentBooks.size();
    int menuRow = -1;
    MappedInputManager::RowTouch menuTouch;
    if (isCarousel) {
      const int menuBottom = renderer.getScreenHeight();
      const int columnWidth = renderer.getScreenWidth() / renderedMenuCount;
      menuTouch = mappedInput.colTouch(menuRow, 0, columnWidth, renderedMenuCount, menuBottom - metrics.menuRowHeight,
                                       menuBottom, columnWidth);
    } else {
      menuTouch = mappedInput.rowTouch(menuRow, menuTop, metrics.menuRowHeight + metrics.menuSpacing,
                                       renderedMenuCount, 0, INT32_MAX, metrics.menuRowHeight);
    }
    if (menuTouch != MappedInputManager::RowTouch::None) {
      const int touchedIndex =
          metrics.homeContinueReadingInMenu ? menuRow : menuRow + static_cast<int>(recentBooks.size());
      if (menuTouch == MappedInputManager::RowTouch::Down) {
        if (selectorIndex != touchedIndex) {
          selectorIndex = touchedIndex;
          if (isCarousel) {
            requestCarouselUpdate(CarouselUpdateScope::MenuOnly);
          } else {
            requestUpdate();
          }
        }
      } else {
        selectorIndex = touchedIndex;
        activateSelection();
      }
      return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activateSelection();
  }

  if (!SETTINGS.standbyShortcutEnabled) {
    sawBackPressInActivity = false;
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    sawBackPressInActivity = true;
  }
  if (sawBackPressInActivity && mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    sawBackPressInActivity = false;
    onStandbyOpen();
  }
}

void HomeActivity::render(RenderLock&&) {
  static_assert(canRenderCarouselMenuOnly(true, true, CarouselUpdateScope::MenuOnly));
  static_assert(!canRenderCarouselMenuOnly(false, true, CarouselUpdateScope::MenuOnly));
  static_assert(!canRenderCarouselMenuOnly(true, false, CarouselUpdateScope::MenuOnly));
  static_assert(!canRenderCarouselMenuOnly(true, true, CarouselUpdateScope::Full));

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const bool isCarousel =
      static_cast<CrossPointSettings::UI_THEME>(SETTINGS.uiTheme) == CrossPointSettings::UI_THEME::LYRA_CAROUSEL;
  const bool gridHome = usesGridHome();

  const int homeMenuItemCount = kHomeMenuItemCount;
  const bool showContinueReading = metrics.homeContinueReadingInMenu && !recentBooks.empty() && !gridHome;
  std::vector<const char*> menuItems;
  std::vector<UIIcon> menuIcons;
  menuItems.reserve(homeMenuItemCount + (showContinueReading ? 1 : 0));
  menuIcons.reserve(homeMenuItemCount + (showContinueReading ? 1 : 0));
  for (int i = 0; i < homeMenuItemCount; ++i) {
    const HomeMenuEntry* entry = menuEntryAtIndex(i, hasOpdsServers, isCarousel);
    menuItems.push_back(I18N.get(entry->label));
    menuIcons.push_back(entry->icon);
  }

  if (showContinueReading) {
    menuItems.insert(menuItems.begin(), tr(STR_CONTINUE_READING));
    menuIcons.insert(menuIcons.begin(), Book);
  }

  const Rect headerRect{0, metrics.topPadding, pageWidth, metrics.homeTopPadding};
  const HomeGridLayout homeGrid = GUI.getHomeGridLayout(renderer, static_cast<int>(menuItems.size()));
  const int homeModuleHeight = GUI.getHomeModuleHeight(renderer, static_cast<int>(menuItems.size()));
  Rect menuRect{0, metrics.homeTopPadding + metrics.homeCoverTileHeight + metrics.homeMenuTopOffset, pageWidth,
                pageHeight - (metrics.headerHeight + metrics.homeTopPadding + metrics.verticalSpacing +
                              metrics.homeMenuTopOffset + metrics.buttonHintsHeight)};
  if (homeGrid.isGrid()) {
    // Grid themes own the home band: bound it to exactly the painted grid so
    // nothing can spill past the screen bottom edge.
    menuRect = Rect{0, homeGrid.cellY, pageWidth,
                    homeGrid.rows * homeGrid.cellHeight + (homeGrid.rows - 1) * homeGrid.gap};
  }
  auto drawHeader = [&] {
    GUI.drawHeader(renderer, headerRect,
                   metrics.homeShowRecentBookTitle && !recentBooks.empty() ? recentBooks[0].title.c_str() : nullptr);
  };
  auto drawMenu = [&] {
    GUI.drawHomeMenu(
        renderer, menuRect, static_cast<int>(menuItems.size()),
        metrics.homeContinueReadingInMenu ? selectorIndex : selectorIndex - recentBooks.size(),
        [&menuItems](int index) { return std::string(menuItems[index]); },
        [&menuIcons](int index) { return menuIcons[index]; });
  };

  const CarouselUpdateScope updateScope = carouselUpdateScope.exchange(CarouselUpdateScope::None);
  const bool menuOnlyUpdate = canRenderCarouselMenuOnly(isCarousel, recentsLoaded, updateScope);
  if (menuOnlyUpdate) {
    renderer.fillRect(headerRect.x, headerRect.y, headerRect.width, headerRect.height, false);
    drawHeader();
    renderer.fillRect(0, pageHeight - metrics.menuRowHeight, pageWidth, metrics.menuRowHeight, false);
    drawMenu();
    renderer.displayBuffer();
    return;
  }

  if (isCarousel) {
    coverRendered = false;
    coverBufferStored = false;
  }

  renderer.clearScreen();
  bool bufferRestored = coverBufferStored && restoreCoverBuffer();

  drawHeader();

  // Record the tile rect so storeCoverBuffer (called from the theme) knows
  // which sub-region of the framebuffer to snapshot. ~16 KB in Portrait
  // instead of the 48 KB full framebuffer the previous bind captured.
  coverRectX = 0;
  coverRectY = metrics.homeTopPadding;
  coverRectW = pageWidth;
  coverRectH = homeModuleHeight;

  GUI.drawRecentBookCover(renderer, Rect{0, metrics.homeTopPadding, pageWidth, homeModuleHeight}, recentBooks,
                          selectorIndex, coverRendered, coverBufferStored, bufferRestored,
                          std::bind(&HomeActivity::storeCoverBuffer, this));

  drawMenu();

  if (!isCarousel && !gridHome) {
    const auto labels = mappedInput.mapLabels(SETTINGS.standbyShortcutEnabled ? tr(STR_STANDBY_TITLE) : "",
                                              tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  renderer.displayBuffer();

  if (!firstRenderDone) {
    firstRenderDone = true;
    requestUpdate();
  } else if (!recentsLoaded && !recentsLoading) {
    recentsLoading = true;
    loadRecentCovers(metrics.homeCoverHeight);
  }
}

void HomeActivity::onSelectBook(const std::string& path) { activityManager.goToReader(path); }

void HomeActivity::onFileBrowserOpen() { activityManager.goToFileBrowser(); }

void HomeActivity::onRecentsOpen() { activityManager.goToRecentBooks(); }

void HomeActivity::onSettingsOpen() { activityManager.goToSettings(); }

void HomeActivity::onFileTransferOpen() { activityManager.goToFileTransfer(); }

void HomeActivity::onOpdsBrowserOpen() { activityManager.goToBrowser(); }

void HomeActivity::onAppsOpen() { activityManager.goToApps(); }

void HomeActivity::onWeReadOpen() {
#ifdef ENABLE_CHINESE_VERSION
  activityManager.goToWeRead();
#endif
}

void HomeActivity::onStandbyOpen() { activityManager.goToStandby(); }
