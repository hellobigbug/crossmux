#include "SettingsActivity.h"

#include <BoardConfig.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <HalStorage.h>
#include <HalSystem.h>
#include <Logging.h>
#if FREEINK_DEVICE_MURPHY_M4 && !defined(SIMULATOR)
#include <HalGPIO.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "AppVisibilitySettingsActivity.h"
#include "ButtonRemapActivity.h"
#include "ClearCacheActivity.h"
#include "CrossPointSettings.h"
#include "DateTimeSettingsActivity.h"
#include "DictionaryDownloadActivity.h"
#include "FontDownloadActivity.h"
#include "InxItemLayout.h"
#include "KOReaderSettingsActivity.h"
#include "LanguageSelectActivity.h"
#include "MappedInputManager.h"
#include "OpdsServerListActivity.h"
#include "OtaUpdateActivity.h"
#include "ReadingStatsSettingsActivity.h"
#include "SdCardFontSystem.h"
#include "SdFirmwareUpdateActivity.h"
#include "SettingsList.h"
#include "SilentRestart.h"
#include "StatusBarSettingsActivity.h"
#include "TextSettingsActivity.h"
#include "activities/home/FileBrowserActivity.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/util/ConfirmationActivity.h"
#include "activities/util/IntervalSelectionActivity.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"
#include "util/ReadingBackground.h"
#include "util/SystemSettingsReset.h"

namespace fui = freeink::ui;

namespace {
constexpr StrId OK_OPTION[] = {StrId::STR_OK_BUTTON};
constexpr uint64_t BYTES_PER_TENTH_GB = 100000000ULL;

enum class AboutRow : uint8_t {
  FirmwareName,
  FirmwareVersion,
  DeviceModel,
  WifiMacAddress,
  ChipTemperature,
  Uptime,
  HeapFreeTotal,
  LargestHeapBlock,
  SdUsedTotal,
  Count,
};

// Icon for every settings row. Action/sub-screen entries are matched by name;
// plain enum/toggle settings fall back to a category glyph so no row is ever
// icon-less.
UIIcon settingsRowIcon(const SettingInfo& setting) {
  switch (setting.nameId) {
    // --- Display ---
    case StrId::STR_SLEEP_SCREEN:
    case StrId::STR_NIGHT_MODE:
      return UIIcon::Moon;
    case StrId::STR_SLEEP_COVER_MODE:
      return UIIcon::Image;
    case StrId::STR_SLEEP_COVER_FILTER:
    case StrId::STR_UI_THEME:
      return UIIcon::Palette;
    case StrId::STR_QUICK_RESUME_TIMEOUT:
      return UIIcon::Zap;
    case StrId::STR_STANDBY_TITLE:
      return UIIcon::Standby;
    case StrId::STR_HIDE_BATTERY:
      return UIIcon::Battery;
    case StrId::STR_REFRESH_FREQ:
      return UIIcon::Refresh;
    case StrId::STR_SUNLIGHT_FADING_FIX:
    case StrId::STR_RESTORE_LIGHT_ON_WAKE:
    case StrId::STR_FRONTLIGHT:
      return UIIcon::Sun;
    case StrId::STR_SHOW_BUTTON_HINTS:
      return UIIcon::Info;
    // --- Reader ---
    case StrId::STR_FONT_FAMILY:
    case StrId::STR_FONT_SIZE:
    case StrId::STR_FAKE_BOLD:
    case StrId::STR_TEXT_SETTINGS:
      return UIIcon::Type;
    case StrId::STR_LINE_SPACING:
    case StrId::STR_PARA_ALIGNMENT:
    case StrId::STR_READING_GUIDE_LINE:
    case StrId::STR_READING_GUIDE_LINE_STYLE:
    case StrId::STR_HYPHENATION:
    case StrId::STR_EXTRA_SPACING:
    case StrId::STR_READER_MENU_STYLE:
      return UIIcon::List;
    case StrId::STR_EMBEDDED_STYLE:
      return UIIcon::BookOpen;
    case StrId::STR_FOCUS_READING:
      return UIIcon::Zap;
    case StrId::STR_READING_BACKGROUND:
    case StrId::STR_TEXT_AA:
      return UIIcon::Palette;
    case StrId::STR_IMAGES:
      return UIIcon::Image;
    case StrId::STR_DAILY_GOAL:
    case StrId::STR_READING_STATS:
      return UIIcon::Database;
    case StrId::STR_ENABLE_ACHIEVEMENTS:
      return UIIcon::Shield;
    case StrId::STR_ACHIEVEMENT_POPUPS:
      return UIIcon::Info;
    case StrId::STR_OPDS_DOWNLOAD_FOLDER:
    case StrId::STR_OPDS_FILENAME_FORMAT:
    case StrId::STR_OPDS_SERVERS:
      return UIIcon::Library;
    case StrId::STR_DOCUMENT_MATCHING:
      return UIIcon::Library;
    case StrId::STR_SEND_METADATA:
      return UIIcon::Upload;
    case StrId::STR_SYNC_BEHAVIOR:
    case StrId::STR_KOREADER_SYNC:
      return UIIcon::Refresh;
    case StrId::STR_CHAPTER_PAGE_COUNT:
    case StrId::STR_BOOK_PROGRESS_PERCENTAGE:
    case StrId::STR_PROGRESS_BAR:
    case StrId::STR_PROGRESS_BAR_THICKNESS:
    case StrId::STR_TITLE:
    case StrId::STR_BATTERY:
    case StrId::STR_XTC_STATUS_BAR:
    case StrId::STR_CLOCK:
    case StrId::STR_CLOCK_FORMAT:
    case StrId::STR_AUTO_TIME:
    case StrId::STR_CUSTOMISE_STATUS_BAR:
      return UIIcon::Monitor;
    case StrId::STR_MANAGE_FONTS:
      return UIIcon::Download;
    case StrId::STR_MANAGE_DICTIONARIES:
      return UIIcon::BookOpen;
    // --- Controls ---
    case StrId::STR_ORIENTATION:
    case StrId::STR_TOUCH_READER_CONTROLS:
    case StrId::STR_FRONT_BTN_FOLLOW_ORIENTATION:
    case StrId::STR_TILT_PAGE_TURN:
      return UIIcon::Smartphone;
    case StrId::STR_SIDE_BTN_LAYOUT:
    case StrId::STR_SHOW_READER_MENU:
      return UIIcon::List;
    case StrId::STR_LONG_PRESS_BEHAVIOR:
    case StrId::STR_SHORT_PWR_BTN:
      return UIIcon::Zap;
    case StrId::STR_LONG_PRESS_MENU:
    case StrId::STR_PWR_BTN_FOOTNOTE_BACK:
      return UIIcon::Bookmark;
    case StrId::STR_BACK_SHORT_TO_FILE_BROWSER:
    case StrId::STR_MOVE_FINISHED_TO_READ:
      return UIIcon::Folder;
    case StrId::STR_SHOW_HIDDEN_FILES:
      return UIIcon::Info;
    case StrId::STR_REMAP_FRONT_BUTTONS:
      return UIIcon::Settings;
    // --- System ---
    case StrId::STR_REMOVE_READ_FROM_RECENTS:
      return UIIcon::Book;
    case StrId::STR_APP_VISIBILITY:
      return UIIcon::Apps;
    case StrId::STR_WIFI_NETWORKS:
      return UIIcon::Wifi;
    case StrId::STR_DATE_AND_TIME:
      return UIIcon::Clock;
    case StrId::STR_CLEAR_READING_CACHE:
      return UIIcon::Database;
    case StrId::STR_RESTORE_SYSTEM_SETTINGS:
    case StrId::STR_CHECK_UPDATES:
      return UIIcon::Refresh;
    case StrId::STR_SD_FIRMWARE_UPDATE:
      return UIIcon::Download;
    case StrId::STR_LANGUAGE:
      return UIIcon::Globe;
    case StrId::STR_ABOUT:
      return UIIcon::Info;
    default:
      break;
  }
  switch (setting.category) {
    case StrId::STR_CAT_DISPLAY:
      return UIIcon::Monitor;
    case StrId::STR_CAT_READER:
      return UIIcon::Book;
    default:
      return UIIcon::Settings;
  }
}

// Settings grid/tab labels use the same size as the Nokia home grid text.
#ifdef ENABLE_CHINESE_VERSION
constexpr int kGridLabelFontId = UI_12_FONT_ID;
#else
constexpr int kGridLabelFontId = NOTOSANS_16_FONT_ID;
#endif

class AboutActivity final : public Activity {
 public:
  AboutActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("About", renderer, mappedInput) {}

  void onEnter() override {
    Activity::onEnter();
    heapInfo = HalSystem::getHeapInfo();
    deviceName = BoardConfig::ACTIVE.name;
    storageAvailable = Storage.getSpace(sdTotalBytes, sdFreeBytes);
#ifdef SIMULATOR
    wifiMacAvailable = HalSystem::getDeviceId(wifiMac);
    uptimeSeconds = millis() / 1000;
#else
    wifiMacAvailable = HalSystem::getWifiStationMac(wifiMac);
    float temperature = 0.0f;
    temperatureAvailable = HalSystem::getChipTemperatureCelsius(temperature);
    if (temperatureAvailable) chipTemperatureCelsius = static_cast<int>(std::lround(temperature));
    uptimeSeconds = HalSystem::getUptimeSeconds();
#endif
    requestUpdate();
  }

  void loop() override {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) finish();
  }

  void render(RenderLock&&) override {
    renderer.clearScreen();
    const auto& metrics = UITheme::getInstance().getMetrics();
    const Rect safeArea = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
    GUI.drawHeader(renderer, Rect{safeArea.x, safeArea.y + metrics.topPadding, safeArea.width, metrics.headerHeight},
                   tr(STR_ABOUT));
    const Rect content{safeArea.x, safeArea.y + metrics.topPadding + metrics.headerHeight, safeArea.width,
                       safeArea.height - metrics.topPadding - metrics.headerHeight - metrics.buttonHintsHeight};
    GUI.drawList(
        renderer, content, static_cast<int>(AboutRow::Count), -1,
        [](const int index) {
          static constexpr StrId LABELS[] = {
              StrId::STR_ABOUT_FIRMWARE_NAME,    StrId::STR_ABOUT_FIRMWARE_VERSION,   StrId::STR_ABOUT_DEVICE_MODEL,
              StrId::STR_ABOUT_WIFI_MAC_ADDRESS, StrId::STR_ABOUT_CHIP_TEMPERATURE,   StrId::STR_ABOUT_UPTIME,
              StrId::STR_ABOUT_HEAP_FREE_TOTAL,  StrId::STR_ABOUT_LARGEST_HEAP_BLOCK, StrId::STR_ABOUT_SD_USED_TOTAL,
          };
          return std::string(I18N.get(LABELS[index]));
        },
        nullptr, nullptr, [this](const int index) { return rowValue(static_cast<AboutRow>(index)); }, false, nullptr);
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer();
  }

 private:
  std::string rowValue(const AboutRow row) const {
    char value[48];
    switch (row) {
      case AboutRow::FirmwareName:
        return tr(STR_CROSSPOINT);
      case AboutRow::FirmwareVersion:
        return CROSSPOINT_VERSION;
      case AboutRow::DeviceModel:
        return deviceName ? deviceName : tr(STR_NOT_AVAILABLE);
      case AboutRow::WifiMacAddress:
        if (!wifiMacAvailable) return tr(STR_NOT_AVAILABLE);
        snprintf(value, sizeof(value), "%02X:%02X:%02X:%02X:%02X:%02X", wifiMac[0], wifiMac[1], wifiMac[2], wifiMac[3],
                 wifiMac[4], wifiMac[5]);
        return value;
      case AboutRow::ChipTemperature:
        if (!temperatureAvailable) return tr(STR_NOT_AVAILABLE);
        snprintf(value, sizeof(value), "%d", chipTemperatureCelsius);
        return value;
      case AboutRow::Uptime: {
        const uint64_t totalMinutes = uptimeSeconds / 60;
        snprintf(value, sizeof(value), "%llu:%02llu:%02llu", static_cast<unsigned long long>(totalMinutes / (24 * 60)),
                 static_cast<unsigned long long>(totalMinutes / 60 % 24),
                 static_cast<unsigned long long>(totalMinutes % 60));
        return value;
      }
      case AboutRow::HeapFreeTotal:
        snprintf(value, sizeof(value), "%lu / %lu", static_cast<unsigned long>(heapInfo.freeBytes / 1024),
                 static_cast<unsigned long>(heapInfo.totalBytes / 1024));
        return value;
      case AboutRow::LargestHeapBlock:
        snprintf(value, sizeof(value), "%lu", static_cast<unsigned long>(heapInfo.largestFreeBlockBytes / 1024));
        return value;
      case AboutRow::SdUsedTotal: {
        if (!storageAvailable) return tr(STR_NOT_AVAILABLE);
        const uint64_t usedTenths = (sdTotalBytes - sdFreeBytes + BYTES_PER_TENTH_GB / 2) / BYTES_PER_TENTH_GB;
        const uint64_t totalTenths = (sdTotalBytes + BYTES_PER_TENTH_GB / 2) / BYTES_PER_TENTH_GB;
        snprintf(value, sizeof(value), "%llu.%llu / %llu.%llu", static_cast<unsigned long long>(usedTenths / 10),
                 static_cast<unsigned long long>(usedTenths % 10), static_cast<unsigned long long>(totalTenths / 10),
                 static_cast<unsigned long long>(totalTenths % 10));
        return value;
      }
      case AboutRow::Count:
        return {};
    }
    return {};
  }

  HalSystem::HeapInfo heapInfo{};
  HalSystem::DeviceId wifiMac{};
  const char* deviceName = nullptr;
  uint64_t uptimeSeconds = 0;
  uint64_t sdTotalBytes = 0;
  uint64_t sdFreeBytes = 0;
  int chipTemperatureCelsius = 0;
  bool wifiMacAvailable = false;
  bool temperatureAvailable = false;
  bool storageAvailable = false;
};
}  // namespace

const StrId SettingsActivity::categoryNames[categoryCount] = {StrId::STR_CAT_DISPLAY, StrId::STR_CAT_READER,
                                                              StrId::STR_CAT_CONTROLS, StrId::STR_CAT_SYSTEM};

SettingsActivity::SettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiTabListActivity("Settings", renderer, mappedInput) {}

void SettingsActivity::selectMainTabContentEdge(const MainTabContentEdge edge) {
  if (usesAccordion()) {
    moveSelectionTo(MainTabs::contentEdgeIndex(edge, listCount()));
    return;
  }
  moveRingTo(MainTabs::contentEdgeIndex(edge, settingsCount) + (settingsCount > 0 ? 1 : 0));
}

void SettingsActivity::rebuildSettingsLists() {
  displaySettings.clear();
  readerSettings.clear();
  controlsSettings.clear();
  systemSettings.clear();

  // Pick up any fonts uploaded/deleted over the web server since the last
  // reader activity ran — otherwise the font-family picker shows stale list.
  sdFontSystem.refreshIfDirty();

  std::vector<DictionaryEntry> dictionaries;
  if (!usesAccordion() || dictionariesLoaded) DictionaryRegistry::discover(dictionaries);

  for (auto& setting : getSettingsList(&sdFontSystem.registry(), &dictionaries)) {
    if (setting.category == StrId::STR_NONE_OPT) continue;
    if (!usesAccordion() && (setting.valuePtr == &CrossPointSettings::inxRecentLayout ||
                             setting.valuePtr == &CrossPointSettings::inxLibraryLayout ||
                             setting.valuePtr == &CrossPointSettings::inxAppsLayout)) {
      continue;
    }
    if (setting.category == StrId::STR_CAT_DISPLAY) {
      // The sunlight fading fix is a grayscale-waveform compensation that does
      // not apply on the X4 Pro / X4 Classic (plain OTP waveform, same panels).
      if (setting.valuePtr == &CrossPointSettings::fadingFix && (BoardConfig::isX4Pro() || FREEINK_DEVICE_X4CLASSIC)) {
        continue;
      }
      displaySettings.push_back(setting);
    } else if (setting.category == StrId::STR_CAT_READER) {
      // Settings merged into "Text Settings"
      // (they stay in the shared list for the web settings API)
      if (setting.inTextSettings || setting.inReadingStatsSettings) continue;
      readerSettings.push_back(setting);
    } else if (setting.category == StrId::STR_CAT_CONTROLS) {
      if (setting.valuePtr == &CrossPointSettings::pwrBtnFootnoteBack &&
          SETTINGS.shortPwrBtn != CrossPointSettings::SHORT_PWRBTN::FOOTNOTES) {
        continue;
      }
      controlsSettings.push_back(setting);
    } else if (setting.category == StrId::STR_CAT_SYSTEM) {
      // These stay in the shared list for persistence and the web API, but the
      // device UI owns them in the Date & Time submenu.
      if (setting.valuePtr == &CrossPointSettings::clockAutoSync ||
          setting.valuePtr == &CrossPointSettings::clockUtcOffsetQ ||
          setting.valuePtr == &CrossPointSettings::clockFormat) {
        continue;
      }
      systemSettings.push_back(setting);
    }
  }

  // Append device-only ACTION items
  if (!BoardConfig::hasTouch()) {
    controlsSettings.insert(controlsSettings.begin(),
                            SettingInfo::Action(StrId::STR_REMAP_FRONT_BUTTONS, SettingAction::RemapFrontButtons));
  }
  systemSettings.push_back(SettingInfo::Action(StrId::STR_APP_VISIBILITY, SettingAction::AppVisibility));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_WIFI_NETWORKS, SettingAction::Network));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_DATE_AND_TIME, SettingAction::DateTime));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_KOREADER_SYNC, SettingAction::KOReaderSync));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_OPDS_SERVERS, SettingAction::OPDSBrowser));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_CLEAR_READING_CACHE, SettingAction::ClearCache));
  systemSettings.push_back(
      SettingInfo::Action(StrId::STR_RESTORE_SYSTEM_SETTINGS, SettingAction::RestoreSystemSettings));
#if FREEINK_DEVICE_MURPHY_M4 && !defined(SIMULATOR)
  systemSettings.push_back(
      SettingInfo::DynamicEnum(
          StrId::STR_M4_HARDWARE_BATCH, {StrId::STR_M4_BATCH_1, StrId::STR_M4_BATCH_2},
          [] { return gpio.murphyM4Batch() == freeink::MurphyM4Batch::First ? 0 : 1; },
          [this](const uint8_t value) {
            const auto batch = value == 0 ? freeink::MurphyM4Batch::First : freeink::MurphyM4Batch::Second;
            if (gpio.saveMurphyM4Batch(batch)) {
              silentRestart();
              return;
            }
            optionPopup.show(StrId::STR_FAILED_LOWER, OK_OPTION, static_cast<int>(std::size(OK_OPTION)), 0, [](int) {});
            requestUpdate();
          })
          .withManagedEnumPicker());
#endif
  // Keep the existing CrossMux OTA proxy flow. Build-only boards compile this
  // UI but are intentionally absent from release assets in this sync.
  systemSettings.push_back(SettingInfo::Action(StrId::STR_CHECK_UPDATES, SettingAction::CheckForUpdates));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_SD_FIRMWARE_UPDATE, SettingAction::SdFirmwareUpdate));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_LANGUAGE, SettingAction::Language));
  systemSettings.push_back(SettingInfo::Action(StrId::STR_ABOUT, SettingAction::About));
  readerSettings.insert(readerSettings.begin(),
                        SettingInfo::Action(StrId::STR_TEXT_SETTINGS, SettingAction::TextSettings));
  readerSettings.insert(readerSettings.begin() + 1,
                        SettingInfo::Action(StrId::STR_MANAGE_FONTS, SettingAction::DownloadFonts));
  readerSettings.insert(readerSettings.begin() + 2,
                        SettingInfo::Action(StrId::STR_MANAGE_DICTIONARIES, SettingAction::ManageDictionaries));
  const auto dictionarySetting =
      std::find_if(readerSettings.begin() + 3, readerSettings.end(),
                   [](const SettingInfo& setting) { return setting.nameId == StrId::STR_DICTIONARY; });
  if (dictionarySetting != readerSettings.end()) {
    std::rotate(readerSettings.begin() + 3, dictionarySetting, dictionarySetting + 1);
  }
  readerSettings.push_back(SettingInfo::Action(StrId::STR_CUSTOMISE_STATUS_BAR, SettingAction::CustomiseStatusBar));
  readerSettings.push_back(SettingInfo::Action(StrId::STR_READING_STATS, SettingAction::ReadingStatsSettings));

  // Update currentSettings pointer and count for the active category
  switch (selectedCategoryIndex) {
    case 0:
      currentSettings = &displaySettings;
      break;
    case 1:
      currentSettings = &readerSettings;
      break;
    case 2:
      currentSettings = &controlsSettings;
      break;
    case 3:
      currentSettings = &systemSettings;
      break;
  }
  settingsCount = static_cast<int>(currentSettings->size());
  rebuildRowItems();
}

void SettingsActivity::onEnter() {
  UiTabListActivity::onEnter();

  // Reset selection to first category (ring position 0, the tab bar, comes
  // from the base's per-tab nav reset)
  selectedCategoryIndex = 0;
  expandedCategories = 0;
  dictionariesLoaded = !usesAccordion();
  preserveQuickResumeTimeoutOn =
      SETTINGS.quickResumeSleepScreen == CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT;
  quickResumeTimeoutAutoEnabled = false;
  syncQuickResumeTimeoutForSleepScreen(/*sleepScreenChanged=*/true, /*quickResumeTimeoutChanged=*/false);

  rebuildSettingsLists();
}

void SettingsActivity::selectCategory(const int categoryIndex) {
  selectedCategoryIndex = categoryIndex;
  switch (selectedCategoryIndex) {
    case 0:
      currentSettings = &displaySettings;
      break;
    case 1:
      currentSettings = &readerSettings;
      break;
    case 2:
      currentSettings = &controlsSettings;
      break;
    case 3:
      currentSettings = &systemSettings;
      break;
  }
  settingsCount = static_cast<int>(currentSettings->size());
  activeNav().top = 0;  // category switches start the list at the top (no per-tab memory here)
  gridSelected_ = 0;
  rebuildRowItems();
}

// Rebuilds rowValues_/rowItems_ (label + actionValue) for *currentSettings.
// Structural — call only when the active category or a category's setting
// list changes, never from buildScreen(), which only refreshes rowValues_
// content and rowItems_[].value pointers in place.
void SettingsActivity::rebuildRowItems() {
  if (usesAccordion()) {
    rebuildAccordionRows();
    return;
  }
  const auto& settings = *currentSettings;
  rowValues_.assign(settings.size(), std::string());
  rowItems_.clear();
  rowItems_.reserve(settings.size());
  for (size_t i = 0; i < settings.size(); i++) {
    fui::ListItem item;
    item.label = I18N.get(settings[i].nameId);
    item.actionValue = static_cast<int16_t>(i);
    item.icon = listIconFor(settingsRowIcon(settings[i]), 32);
    rowItems_.push_back(item);
  }
}

int SettingsActivity::listCount() const {
  return usesAccordion() ? InxAccordionGeometry::visibleCount(accordionSettingCounts(), expandedCategories)
                         : settingsCount;
}

freeink::ui::ListNav& SettingsActivity::activeNav() { return usesAccordion() ? nav : UiTabListActivity::activeNav(); }

void SettingsActivity::rebuildAccordionRows() {
  const auto counts = accordionSettingCounts();
  const int count = InxAccordionGeometry::visibleCount(counts, expandedCategories);
  rowLabels_.resize(static_cast<size_t>(count));
  rowValues_.resize(static_cast<size_t>(count));
  rowItems_.clear();
  rowItems_.reserve(static_cast<size_t>(count));
  for (int index = 0; index < count; ++index) {
    const auto row = InxAccordionGeometry::rowAt(counts, expandedCategories, index);
    const bool category = row.isCategory();
    rowLabels_[index] = category ? I18N.get(categoryNames[row.category])
                                 : std::string("  ") + I18N.get(settingsForCategory(row.category)[row.setting].nameId);
    fui::ListItem item;
    item.label = rowLabels_[index].c_str();
    item.actionValue = static_cast<int16_t>(index);
    if (category) {
      item.state = fui::StateEmphasized;
      item.icon = listIconFor(UIIcon::Settings, 32);
    } else {
      item.icon = listIconFor(settingsRowIcon(settingsForCategory(row.category)[row.setting]), 32);
    }
    rowItems_.push_back(item);
  }
}

void SettingsActivity::onTabAction(const int index) {
  if (optionPopup.isActive()) return;
  selectCategory(index);
  activeNav().selected = 0;  // tab taps land with the tab bar focused
  // The switched-to tab repaints as the selected pill; a flash overlay on top
  // of it just repaints the pill in the focused style.
  app.clearTapFlash();
}

void SettingsActivity::activateIndex(const int index) {
  if (optionPopup.isActive()) return;
  if (usesAccordion()) {
    const auto row = InxAccordionGeometry::rowAt(accordionSettingCounts(), expandedCategories, index);
    if (row.isCategory())
      toggleAccordionCategory(row.category);
    else
      toggleAccordionSetting(row.category, row.setting);
    app.clearTapFlash();
    return;
  }
  (void)index;  // toggleCurrentSetting reads the ring position
  // Most rows repaint a different surface (popup, sub-activity, new value);
  // a lingering tap flash would gray an unrelated element.
  app.clearTapFlash();
  toggleCurrentSetting();
  // Tap-first: a tapped row is not a cursor position. Leaving it focused
  // (inverted) after the tap meant the row stayed black once its sub-screen or
  // popup closed, and Back then had to clear that focus before a second Back
  // left Settings. Hand the focus back to the tab band; the viewport stays put.
  if (mappedInput.hasTouch()) {
    activeNav().selected = 0;
  }
}

void SettingsActivity::onRowAction(const fui::ActionEvent& event) {
  if (!usesAccordion()) {
    UiTabListActivity::onRowAction(event);
    return;
  }
  nav.selected = event.value;
  activateIndex(event.value);
}

void SettingsActivity::onExit() {
  Activity::onExit();

  UITheme::getInstance().reload();  // Re-apply theme in case it was changed
}

void SettingsActivity::applyUiSettingChange(uint8_t CrossPointSettings::* valuePtr) {
  // Theme changes take effect immediately, on this screen — reload the theme
  // and re-derive the app's tokens so the very next repaint is in the new look.
  if (valuePtr != &CrossPointSettings::uiTheme) {
    return;
  }
  RenderLock lock(*this);
  UITheme::getInstance().reload();
  expandedCategories = 0;
  nav.reset();
  rebuildSettingsLists();
  // Re-derive the shared tokens for the new look; the gate stays closed until
  // the repaint that rebuilds the interaction table in the new layout.
  resetUi();
}

bool SettingsActivity::handleCustomInput() {
  if (optionPopup.handleInput(mappedInput, [this] { requestUpdate(); })) return true;
  if (!usesGridLayout()) return false;

  const int count = settingsCount;
  const HomeGridLayout layout = settingsGridLayout();
  if (count <= 0 || !layout.isGrid()) return false;

  const auto& metrics = UITheme::getInstance().getMetrics();
  constexpr int kTabGap = 8;
  constexpr int kTabHeight = 52;
  const int pageWidth = renderer.getScreenWidth();
  const int tabY = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int tabX0 = metrics.contentSidePadding;
  const int tabWidth = (pageWidth - tabX0 * 2 - kTabGap * (categoryCount - 1)) / categoryCount;

  int downX = 0;
  int downY = 0;
  int tapX = 0;
  int tapY = 0;
  const bool touched = mappedInput.wasScreenTouchDown(downX, downY);
  const bool tapped = mappedInput.wasScreenTapped(tapX, tapY);

  // Category tab taps switch the active category.
  if (tapped && tapY >= tabY && tapY < tabY + kTabHeight) {
    const int c = (tapX - tabX0) / (tabWidth + kTabGap);
    if (c >= 0 && c < categoryCount && c != selectedCategoryIndex) {
      selectCategory(c);
      gridSelected_ = 0;
      requestUpdate();
    }
    return true;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    SETTINGS.saveToFile();
    onGoHome();
    return true;
  }

  // Direction buttons walk the grid; leaving the top/bottom edge moves to the
  // neighbouring category.
  if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    gridSelected_ = std::min(gridSelected_ + 1, count - 1);
    requestUpdate();
    return true;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    gridSelected_ = std::max(0, gridSelected_ - 1);
    requestUpdate();
    return true;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
    if (gridSelected_ + layout.columns >= count) {
      selectCategory((selectedCategoryIndex + 1) % categoryCount);
      gridSelected_ = 0;
    } else {
      gridSelected_ += layout.columns;
    }
    requestUpdate();
    return true;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
    if (gridSelected_ - layout.columns < 0) {
      const int prev = (selectedCategoryIndex + categoryCount - 1) % categoryCount;
      selectCategory(prev);
      gridSelected_ = std::max(0, static_cast<int>(settingsForCategory(prev).size()) - 1);
    } else {
      gridSelected_ -= layout.columns;
    }
    requestUpdate();
    return true;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activateGridSetting();
    return true;
  }

  if (touched) {
    const int hit = layout.indexFromPoint(downX, downY, count);
    if (hit >= 0 && hit != gridSelected_) {
      gridSelected_ = hit;
      requestUpdate();
    }
    return true;
  }
  if (tapped) {
    const int hit = layout.indexFromPoint(tapX, tapY, count);
    if (hit >= 0) {
      gridSelected_ = hit;
      activateGridSetting();
    }
    return true;
  }

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
    const int step = (swipe == MappedInputManager::SwipeDir::Down) ? layout.columns : -layout.columns;
    gridSelected_ = std::clamp(gridSelected_ + step, 0, count - 1);
    requestUpdate();
    return true;
  }
  return false;
}

void SettingsActivity::navigateButtons() {
  if (usesAccordion()) {
    UiListActivity::navigateButtons();
    return;
  }
  UiTabListActivity::navigateButtons();
}

void SettingsActivity::stepTab(const int direction) {
  // Ring position 0 stays on the tab bar; a row selection collapses to the
  // new category's first row (per-tab memory is deliberately not kept here).
  const bool onTabBar = ringPos() == 0;
  selectedCategoryIndex = direction > 0 ? ButtonNavigator::nextIndex(selectedCategoryIndex, categoryCount)
                                        : ButtonNavigator::previousIndex(selectedCategoryIndex, categoryCount);
  selectCategory(selectedCategoryIndex);
  activeNav().selected = onTabBar ? 0 : 1;
  requestUpdate();
}

bool SettingsActivity::handleButtons() {
  if (usesAccordion()) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      activateIndex(nav.selected);
      return true;
    }
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      if (expandedCategories != 0) {
        const auto row = InxAccordionGeometry::rowAt(accordionSettingCounts(), expandedCategories, nav.selected);
        const uint8_t mask = row.category >= 0 ? static_cast<uint8_t>(uint8_t{1} << row.category) : 0;
        expandedCategories =
            (mask != 0 && (expandedCategories & mask) != 0) ? static_cast<uint8_t>(expandedCategories & ~mask) : 0;
        rebuildAccordionRows();
        nav.selected = std::min(nav.selected, listCount() - 1);
        nav.follow(listCount());
        requestUpdate();
      }
      return true;
    }
    return false;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (ringPos() == 0) {
      stepTab(1);
    } else {
      toggleCurrentSetting();
      requestUpdate();
    }
    return true;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (ringPos() > 0) {
      activeNav().selected = 0;
      requestUpdate();
    } else {
      SETTINGS.saveToFile();
      onGoHome();
    }
    return true;
  }

  return false;
}

bool SettingsActivity::usesAccordion() const { return UITheme::getInstance().hasMainTabs(); }

bool SettingsActivity::usesGridLayout() const {
  // The 552×768 design contract uses a list-style settings screen
  // (segmented tabs + outlined rows), so the Nokia grid is disabled.
  return false;
}

const char* SettingsActivity::tabLabel(const int index) const {
  // On the small-screen grid layout the Reader tab uses the short label so all
  // four tabs fit without crowding.
  if (usesGridLayout() && index == 1) return I18N.get(StrId::STR_CAT_READER_SHORT);
  return I18N.get(categoryNames[index]);
}

HomeGridLayout SettingsActivity::settingsGridLayout() const {
  HomeGridLayout layout;
  const int count = settingsCount;
  if (count <= 0) return layout;

  constexpr int kColumns = 3;
  constexpr int kGap = 10;
  constexpr int kSidePadding = 24;
  constexpr int kTabBandHeight = 52;
  constexpr int kTabGap = 8;
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int top = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + kTabBandHeight + kTabGap;
  const int bottom = pageHeight - metrics.buttonHintsHeight - kTabGap;
  if (bottom <= top) return layout;

  const int gridWidth = pageWidth - kSidePadding * 2;
  const int cellWidth = (gridWidth - (kColumns - 1) * kGap) / kColumns;
  const int rows = (count + kColumns - 1) / kColumns;
  const int availableHeight = bottom - top;
  const int cellHeight = std::min(120, (availableHeight - (rows - 1) * kGap) / rows);
  if (cellWidth <= 0 || cellHeight <= 0) return layout;

  const int gridHeight = rows * cellHeight + (rows - 1) * kGap;
  layout.columns = kColumns;
  layout.rows = rows;
  layout.cellX = (pageWidth - gridWidth) / 2;
  // Buttons hug the top of the grid band: top-aligned, never centered.
  layout.cellY = top;
  layout.cellWidth = cellWidth;
  layout.cellHeight = cellHeight;
  layout.gap = kGap;
  return layout;
}

void SettingsActivity::drawSettingsGrid(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();

  // Category tabs: four small Nokia pills in one row.
  constexpr int kTabGap = 8;
  constexpr int kTabHeight = 52;
  const int tabY = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int tabX0 = metrics.contentSidePadding;
  const int tabWidth = (pageWidth - tabX0 * 2 - kTabGap * (categoryCount - 1)) / categoryCount;
  for (int c = 0; c < categoryCount; ++c) {
    const int x = tabX0 + c * (tabWidth + kTabGap);
    const bool selected = c == selectedCategoryIndex;
    renderer.fillRoundedRect(x, tabY, tabWidth, kTabHeight, 26, selected ? Color::Black : Color::White);
    if (!selected) renderer.drawRoundedRect(x, tabY, tabWidth, kTabHeight, 1, 26, true);
    const char* label = tabLabel(c);
    const int labelW = renderer.getTextWidth(kGridLabelFontId, label, EpdFontFamily::BOLD);
    const int labelY = tabY + (kTabHeight - renderer.getLineHeight(kGridLabelFontId)) / 2;
    renderer.drawText(kGridLabelFontId, x + (tabWidth - labelW) / 2, labelY, label, !selected, EpdFontFamily::BOLD);
  }

  // Setting tiles: icon + label, selected inverts (Nokia soft-key style).
  const HomeGridLayout layout = settingsGridLayout();
  if (!layout.isGrid()) return;
  const auto& settings = *currentSettings;
  for (int i = 0; i < static_cast<int>(settings.size()); ++i) {
    const int col = i % layout.columns;
    const int row = i / layout.columns;
    const int x = layout.cellX + col * (layout.cellWidth + layout.gap);
    const int y = layout.cellY + row * (layout.cellHeight + layout.gap);
    const bool isSelected = i == gridSelected_;
    renderer.fillRoundedRect(x, y, layout.cellWidth, layout.cellHeight, 26,
                             isSelected ? Color::Black : Color::White);
    if (!isSelected) renderer.drawRoundedRect(x, y, layout.cellWidth, layout.cellHeight, 1, 26, true);

    const std::string label = renderer.truncatedText(kGridLabelFontId, I18N.get(settings[i].nameId),
                                                     std::max(1, layout.cellWidth - 14), EpdFontFamily::BOLD);
    const int labelW = renderer.getTextWidth(kGridLabelFontId, label.c_str(), EpdFontFamily::BOLD);
    const int labelLine = renderer.getLineHeight(kGridLabelFontId);
    if (rowItems_[i].icon) {
      // Icon + label as one block, centered in the tile (both axes).
      const int iconSize = rowItems_[i].icon.width;
      constexpr int kIconLabelGap = 8;
      const int blockH = iconSize + kIconLabelGap + labelLine;
      const int iconY = y + 12;
      const int iconX = x + (layout.cellWidth - iconSize) / 2;
      screen.target().bitmap(
          freeink::ui::Rect{static_cast<int16_t>(iconX), static_cast<int16_t>(iconY),
                            static_cast<int16_t>(iconSize), static_cast<int16_t>(iconSize)},
          rowItems_[i].icon, freeink::ui::BitmapMode::Contain,
          freeink::ui::Paint::solid(isSelected ? freeink::ui::Color::White : freeink::ui::Color::Black));
      renderer.drawText(kGridLabelFontId, x + (layout.cellWidth - labelW) / 2, iconY + iconSize + kIconLabelGap,
                        label.c_str(), !isSelected, EpdFontFamily::BOLD);
    } else {
      renderer.drawText(kGridLabelFontId, x + (layout.cellWidth - labelW) / 2,
                        y + 12, label.c_str(), !isSelected, EpdFontFamily::BOLD);
    }
  }
}

void SettingsActivity::activateGridSetting() {
  if (gridSelected_ < 0 || gridSelected_ >= settingsCount) return;
  // Reuse the list activation path, which reads the ring position.
  tabNavs[static_cast<size_t>(selectedCategoryIndex)].selected = gridSelected_ + 1;
  app.clearTapFlash();
  toggleCurrentSetting();
  requestUpdate();
}

const std::vector<SettingInfo>& SettingsActivity::settingsForCategory(const int categoryIndex) const {
  switch (categoryIndex) {
    case 0:
      return displaySettings;
    case 1:
      return readerSettings;
    case 2:
      return controlsSettings;
    case 3:
      return systemSettings;
  }
  return displaySettings;
}

std::array<int, SettingsActivity::categoryCount> SettingsActivity::accordionSettingCounts() const {
  return {static_cast<int>(displaySettings.size()), static_cast<int>(readerSettings.size()),
          static_cast<int>(controlsSettings.size()), static_cast<int>(systemSettings.size())};
}

void SettingsActivity::toggleAccordionCategory(const int categoryIndex) {
  if (categoryIndex < 0 || categoryIndex >= categoryCount) return;
  const uint8_t mask = static_cast<uint8_t>(uint8_t{1} << categoryIndex);
  if (categoryIndex == 1 && !dictionariesLoaded && (expandedCategories & mask) == 0) {
    dictionariesLoaded = true;
    rebuildSettingsLists();
  }
  expandedCategories ^= mask;
  rebuildAccordionRows();
  nav.selected = InxAccordionGeometry::categoryRow(accordionSettingCounts(), expandedCategories, categoryIndex);
  nav.follow(listCount());
  requestUpdate();
}

void SettingsActivity::toggleAccordionSetting(const int categoryIndex, const int settingIndex) {
  if (categoryIndex < 0 || categoryIndex >= categoryCount) return;
  const auto& settings = settingsForCategory(categoryIndex);
  if (settingIndex < 0 || settingIndex >= static_cast<int>(settings.size())) return;
  selectedCategoryIndex = categoryIndex;
  currentSettings = &settings;
  settingsCount = static_cast<int>(settings.size());
  tabNavs[static_cast<size_t>(categoryIndex)].selected = settingIndex + 1;
  toggleCurrentSetting();
  if (!usesAccordion()) return;
  rebuildAccordionRows();
  nav.selected =
      InxAccordionGeometry::categoryRow(accordionSettingCounts(), expandedCategories, categoryIndex) + 1 + settingIndex;
  nav.follow(listCount());
  requestUpdate();
}

void SettingsActivity::toggleCurrentSetting() {
  int selectedSetting = ringPos() - 1;
  if (selectedSetting < 0 || selectedSetting >= settingsCount) {
    return;
  }

  const auto& setting = (*currentSettings)[selectedSetting];
  const auto changedValuePtr = setting.valuePtr;
  const bool sleepScreenChanged = setting.valuePtr == &CrossPointSettings::sleepScreen;
  const bool quickResumeTimeoutChanged = setting.valuePtr == &CrossPointSettings::quickResumeSleepScreen;

  if (setting.nameId == StrId::STR_TIME_TO_SLEEP) {
    openSleepTimeoutPicker();
    return;
  }

  if (setting.valuePtr == &CrossPointSettings::readingBackgroundEnabled) {
    openReadingBackgroundMenu();
    return;
  }

  if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
    // Every toggle opens the modal picker: Off / On with the current state
    // preselected. Applying is immediate, saved, and rebuilds the list.
    static constexpr StrId TOGGLE_OPTIONS[] = {StrId::STR_STATE_OFF, StrId::STR_STATE_ON};
    const auto valuePtr = setting.valuePtr;
    optionPopup.show(setting.nameId, TOGGLE_OPTIONS, 2, SETTINGS.*(valuePtr) ? 1 : 0,
                     [this, valuePtr, sleepScreenChanged, quickResumeTimeoutChanged](int idx) {
                       SETTINGS.*valuePtr = idx != 0;
                       syncQuickResumeTimeoutForSleepScreen(sleepScreenChanged, quickResumeTimeoutChanged);
                       SETTINGS.saveToFile();
                       rebuildSettingsLists();
                     });
    requestUpdate();
    return;
  } else if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
    const uint8_t currentValue = SETTINGS.*(setting.valuePtr);
    const auto valuePtr = setting.valuePtr;
    optionPopup.show(setting.nameId, setting.enumValues.data(), static_cast<int>(setting.enumValues.size()),
                     currentValue, [this, valuePtr, sleepScreenChanged, quickResumeTimeoutChanged](int idx) {
                       SETTINGS.*valuePtr = idx;
                       syncQuickResumeTimeoutForSleepScreen(sleepScreenChanged, quickResumeTimeoutChanged);
                       SETTINGS.saveToFile();
                       if (valuePtr == &CrossPointSettings::uiTheme)
                         applyUiSettingChange(valuePtr);
                       else
                         rebuildSettingsLists();
                     });
    requestUpdate();
    return;
  } else if (setting.type == SettingType::ENUM && setting.valueGetter && setting.valueSetter) {
    const uint8_t totalValues = setting.enumStringValues.empty()
                                    ? static_cast<uint8_t>(setting.enumValues.size())
                                    : static_cast<uint8_t>(setting.enumStringValues.size());
    const uint8_t cur = setting.valueGetter();
    if (setting.managedEnumPicker || totalValues >= 2) {
      const auto valueSetter = setting.valueSetter;
      const bool managedPicker = setting.managedEnumPicker;
      auto onSelect = [this, valueSetter, sleepScreenChanged, quickResumeTimeoutChanged, managedPicker,
                       cur](const int idx) {
        if (idx == cur) return;
        valueSetter(static_cast<uint8_t>(idx));
        if (managedPicker) return;
        syncQuickResumeTimeoutForSleepScreen(sleepScreenChanged, quickResumeTimeoutChanged);
        SETTINGS.saveToFile();
        rebuildSettingsLists();
      };
      if (!setting.enumStringValues.empty()) {
        optionPopup.show(setting.nameId, setting.enumStringValues, cur, std::move(onSelect));
      } else {
        optionPopup.show(setting.nameId, setting.enumValues.data(), static_cast<int>(setting.enumValues.size()), cur,
                         std::move(onSelect));
      }
      requestUpdate();
      return;
    }
    setting.valueSetter((cur + 1) % totalValues);
  } else if (setting.type == SettingType::VALUE && setting.valuePtr != nullptr) {
    const int8_t currentValue = SETTINGS.*(setting.valuePtr);
    if (currentValue + setting.valueRange.step > setting.valueRange.max) {
      SETTINGS.*(setting.valuePtr) = setting.valueRange.min;
    } else {
      SETTINGS.*(setting.valuePtr) = currentValue + setting.valueRange.step;
    }
  } else if (setting.type == SettingType::ACTION) {
    // Every child page returns here through ActivityManager's Pop; make sure the
    // settings list is both rebuilt (values may have changed) and repainted.
    auto resultHandler = [this](const ActivityResult&) {
      SETTINGS.saveToFile();
      rebuildSettingsLists();
      requestUpdate();
    };

    switch (setting.action) {
      case SettingAction::RemapFrontButtons:
        startActivityForResultWith<ButtonRemapActivity>(resultHandler);
        break;
      case SettingAction::CustomiseStatusBar:
        startActivityForResultWith<StatusBarSettingsActivity>(resultHandler);
        break;
      case SettingAction::ReadingStatsSettings:
        startActivityForResultWith<ReadingStatsSettingsActivity>(resultHandler);
        break;
      case SettingAction::AppVisibility:
        startActivityForResultWith<AppVisibilitySettingsActivity>(resultHandler);
        break;
      case SettingAction::KOReaderSync:
        startActivityForResultWith<KOReaderSettingsActivity>(resultHandler);
        break;
      case SettingAction::OPDSBrowser:
        startActivityForResultWith<OpdsServerListActivity>(resultHandler);
        break;
      case SettingAction::Network:
        startActivityForResultWith<WifiSelectionActivity>(resultHandler, false);
        break;
      case SettingAction::DateTime:
        startActivityForResultWith<DateTimeSettingsActivity>(resultHandler);
        break;
      case SettingAction::ClearCache:
        startActivityForResultWith<ClearCacheActivity>(resultHandler);
        break;
      case SettingAction::RestoreSystemSettings:
        confirmRestoreSystemSettings();
        break;
      case SettingAction::CheckForUpdates:
        openOtaUpdate();
        break;
      case SettingAction::SdFirmwareUpdate:
        startActivityForResultWith<SdFirmwareUpdateActivity>(resultHandler);
        break;
      case SettingAction::DownloadFonts:
        startActivityForResultWith<FontDownloadActivity>([this](const ActivityResult&) {
          SETTINGS.saveToFile();
          rebuildSettingsLists();
          requestUpdate();
        });
        break;
      case SettingAction::ManageDictionaries:
        startActivityForResultWith<DictionaryDownloadActivity>([this](const ActivityResult&) {
          SETTINGS.saveToFile();
          rebuildSettingsLists();
          requestUpdate();
        });
        break;
      case SettingAction::TextSettings:
        startActivityForResultWith<TextSettingsActivity>(
            [this](const ActivityResult&) {
              // TextSettingsActivity saves on each change; no save needed here.
              rebuildSettingsLists();
              requestUpdate();
            },
            &sdFontSystem.registry(), TextSettingsActivity::Tab::Family);
        break;
      case SettingAction::Language:
        // Row labels are translated once in rebuildRowItems() and don't
        // re-run on Pop (see ActivityManager::loop()), so a language switch
        // needs an explicit rebuild here rather than the generic resultHandler.
        startActivityForResultWith<LanguageSelectActivity>([this](const ActivityResult&) {
          SETTINGS.saveToFile();
          rebuildSettingsLists();
        });
        break;
      case SettingAction::About:
        startActivityForResultWith<AboutActivity>(resultHandler);
        break;
      case SettingAction::None:
        // Do nothing
        break;
    }
    return;  // Results will be handled in the result handler, so we can return early here
  } else {
    return;
  }

  syncQuickResumeTimeoutForSleepScreen(sleepScreenChanged, quickResumeTimeoutChanged);
  SETTINGS.saveToFile();
  if (changedValuePtr == &CrossPointSettings::uiTheme) {
    applyUiSettingChange(changedValuePtr);
  } else {
    rebuildSettingsLists();
    activeNav().selected = std::min(ringPos(), settingsCount);
  }
}

void SettingsActivity::confirmRestoreSystemSettings() {
  const bool started = startActivityForResultWith<ConfirmationActivity>(
      [this](const ActivityResult& firstResult) {
        if (firstResult.isCancelled) return;

        const bool finalStarted = startActivityForResultWith<ConfirmationActivity>(
            [this](const ActivityResult& finalResult) {
              if (finalResult.isCancelled) return;
              if (systemSettingsReset::clearPersistedSettings()) {
                silentRestart();
                return;
              }
              optionPopup.show(StrId::STR_RESTORE_SYSTEM_SETTINGS_FAILED, OK_OPTION,
                               static_cast<int>(std::size(OK_OPTION)), 0, [](int) {});
              requestUpdate();
            },
            tr(STR_RESTORE_SYSTEM_SETTINGS), tr(STR_RESTORE_SYSTEM_SETTINGS_FINAL_WARNING));
        if (finalStarted) return;
        optionPopup.show(StrId::STR_MEMORY_ERROR, OK_OPTION, static_cast<int>(std::size(OK_OPTION)), 0, [](int) {});
        requestUpdate();
      },
      tr(STR_RESTORE_SYSTEM_SETTINGS), tr(STR_RESTORE_SYSTEM_SETTINGS_WARNING));
  if (started) return;
  optionPopup.show(StrId::STR_MEMORY_ERROR, OK_OPTION, static_cast<int>(std::size(OK_OPTION)), 0, [](int) {});
  requestUpdate();
}

void SettingsActivity::syncQuickResumeTimeoutForSleepScreen(bool sleepScreenChanged, bool quickResumeTimeoutChanged) {
  if (quickResumeTimeoutChanged) {
    preserveQuickResumeTimeoutOn =
        SETTINGS.quickResumeSleepScreen == CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT;
    quickResumeTimeoutAutoEnabled = false;
  }

  if (SETTINGS.sleepScreen == CrossPointSettings::SLEEP_SCREEN_MODE::QUICK_RESUME) {
    if (SETTINGS.quickResumeSleepScreen != CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT) {
      SETTINGS.quickResumeSleepScreen = CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_AFTER_TIMEOUT;
      quickResumeTimeoutAutoEnabled = !preserveQuickResumeTimeoutOn;
    } else if (sleepScreenChanged && !preserveQuickResumeTimeoutOn) {
      quickResumeTimeoutAutoEnabled = true;
    }
    return;
  }

  if (sleepScreenChanged && quickResumeTimeoutAutoEnabled && !preserveQuickResumeTimeoutOn) {
    SETTINGS.quickResumeSleepScreen = CrossPointSettings::QUICK_RESUME_SLEEP_SCREEN::QUICK_RESUME_NEVER;
    quickResumeTimeoutAutoEnabled = false;
  }
}

void SettingsActivity::openOtaUpdate() {
  {
    RenderLock lock(*this);
    closeRouting();
    // clear() keeps vector capacity; swap it out so the stacked Settings activity leaves that heap to TLS.
    std::vector<freeink::ui::ListItem>().swap(rowItems_);
    std::vector<std::string>().swap(rowValues_);
    std::vector<std::string>().swap(rowLabels_);
    std::vector<SettingInfo>().swap(displaySettings);
    std::vector<SettingInfo>().swap(readerSettings);
    std::vector<SettingInfo>().swap(controlsSettings);
    std::vector<SettingInfo>().swap(systemSettings);
    settingsCount = 0;
  }

  if (startActivityForResultWith<OtaUpdateActivity>([this](const ActivityResult&) {
        SETTINGS.saveToFile();
        rebuildSettingsLists();
      })) {
    return;
  }

  rebuildSettingsLists();
  requestUpdate();
}

void SettingsActivity::openSleepTimeoutPicker() {
  startActivityForResultWith<IntervalSelectionActivity>(
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          SETTINGS.sleepTimeoutMinutes = static_cast<uint8_t>(std::get<IntervalResult>(result.data).value);
          SETTINGS.saveToFile();
        }
        requestUpdate();
      },
      "SleepTimeoutInterval", StrId::STR_TIME_TO_SLEEP, SETTINGS.sleepTimeoutMinutes,
      CrossPointSettings::MIN_SLEEP_TIMEOUT_MINUTES, CrossPointSettings::MAX_SLEEP_TIMEOUT_MINUTES, 1, 5,
      StrId::STR_SLEEP_TIMER_VALUE_FORMAT, false, StrId::STR_SLEEP_NEVER);
}

void SettingsActivity::openReadingBackgroundMenu() {
  static constexpr StrId OPTIONS[] = {StrId::STR_STATE_OFF, StrId::STR_CUSTOM_IMAGE};
  optionPopup.show(StrId::STR_READING_BACKGROUND, OPTIONS, static_cast<int>(std::size(OPTIONS)),
                   SETTINGS.readingBackgroundEnabled ? 1 : 0, [this](const int selected) {
                     if (selected == 0) {
                       SETTINGS.readingBackgroundEnabled = 0;
                       SETTINGS.saveToFile();
                       requestUpdate();
                       return;
                     }
                     openReadingBackgroundPicker();
                   });
  requestUpdate();
}

void SettingsActivity::openReadingBackgroundPicker() {
  const bool started = startActivityForResultWith<FileBrowserActivity>(
      [this](const ActivityResult& result) {
        if (result.isCancelled) return;
        const auto* selected = std::get_if<FilePathResult>(&result.data);
        if (!selected) {
          LOG_ERR("SET", "PNG picker returned no path");
          return;
        }

        GUI.drawPopup(renderer, tr(STR_LOADING_POPUP));
        if (readingBackground::createCacheFromPng(renderer, selected->path.c_str())) {
          SETTINGS.readingBackgroundEnabled = 1;
          SETTINGS.saveToFile();
          requestUpdate();
        } else {
          optionPopup.show(StrId::STR_FAILED_LOWER, OK_OPTION, static_cast<int>(std::size(OK_OPTION)), 0, [](int) {});
        }
      },
      "/", FileBrowserActivity::Mode::PickPng);
  if (!started) {
    optionPopup.show(StrId::STR_MEMORY_ERROR, OK_OPTION, static_cast<int>(std::size(OK_OPTION)), 0, [](int) {});
    requestUpdate();
  }
}

std::string SettingsActivity::settingValueText(const SettingInfo& setting) {
  if (setting.valuePtr == &CrossPointSettings::readingBackgroundEnabled) {
    return SETTINGS.readingBackgroundEnabled ? tr(STR_CUSTOM_IMAGE) : tr(STR_STATE_OFF);
  }
  if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
    return SETTINGS.*(setting.valuePtr) ? tr(STR_STATE_ON) : tr(STR_STATE_OFF);
  }
  if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
    // Guard like the valueGetter branch below: a corrupt/migrated settings
    // byte must not index past the enum table.
    const uint8_t value = SETTINGS.*(setting.valuePtr);
    if (value >= setting.enumValues.size()) return "";
    return I18N.get(setting.enumValues[value]);
  }
  if (setting.type == SettingType::ENUM && setting.valueGetter) {
    const uint8_t value = setting.valueGetter();
    if (!setting.enumStringValues.empty() && value < setting.enumStringValues.size()) {
      return setting.enumStringValues[value];
    }
    if (value < setting.enumValues.size()) {
      return I18N.get(setting.enumValues[value]);
    }
    return "";
  }
  if (setting.type == SettingType::VALUE && setting.valuePtr != nullptr) {
    if (setting.nameId == StrId::STR_TIME_TO_SLEEP) {
      if (SETTINGS.sleepTimeoutMinutes >= CrossPointSettings::SLEEP_TIMEOUT_NEVER_MINUTES) {
        return tr(STR_SLEEP_NEVER);
      }
      char valueBuffer[32];
      snprintf(valueBuffer, sizeof(valueBuffer), tr(STR_SLEEP_TIMER_VALUE_FORMAT),
               static_cast<unsigned int>(SETTINGS.*(setting.valuePtr)));
      return valueBuffer;
    }
    return std::to_string(SETTINGS.*(setting.valuePtr));
  }
  return "";
}

void SettingsActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const bool boldChineseCategories = I18N.getLanguage() == Language::ZH_CN;
  // Content below the GUI.drawHeader band, above the button hints.
  screen.setContentMargin(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                      static_cast<int16_t>(metrics.buttonHintsHeight), 0});

  if (usesAccordion()) {
    const auto counts = accordionSettingCounts();
    for (int i = 0; i < listCount(); ++i) {
      const auto row = InxAccordionGeometry::rowAt(counts, expandedCategories, i);
      rowValues_[i] = row.isCategory() ? ((expandedCategories & (uint8_t{1} << row.category)) != 0 ? "-" : "+")
                                       : settingValueText(settingsForCategory(row.category)[row.setting]);
      rowItems_[i].value = rowValues_[i].empty() ? nullptr : rowValues_[i].c_str();
    }
    fui::ListProps props;
    props.items = rowItems_.data();
    props.count = static_cast<uint16_t>(rowItems_.size());
    props.action = ACTION_ROW;
    props.inputMask = fui::InputTouch;
    props.labelText = screen.theme().bodyText;
    props.valueText = screen.theme().bodyText;
    syncListViewport(screen, props);
    if (!showMainTabContentSelection()) props.selectedIndex = -1;
    if (boldChineseCategories) {
      props.labelText.bold = false;
      props.valueText.bold = false;
    }
    GfxRenderer::SyntheticBoldScope syntheticBold(renderer, boldChineseCategories
                                                                ? CrossPointSettings::SYNTHETIC_BOLD_STANDARD
                                                                : CrossPointSettings::SYNTHETIC_BOLD_OFF);
    screen.list(props);
    return;
  }

  if (usesGridLayout()) {
    drawSettingsGrid(screen);
    return;
  }

  {
    GfxRenderer::SyntheticBoldScope syntheticBold(renderer, boldChineseCategories
                                                                ? CrossPointSettings::SYNTHETIC_BOLD_STANDARD
                                                                : CrossPointSettings::SYNTHETIC_BOLD_OFF);
    buildTabBar(screen, boldChineseCategories);
  }

  // rowItems_ (label/actionValue) was built by rebuildRowItems() when the
  // category was last selected/rebuilt; only the live value text needs
  // refreshing here, by assigning into the existing rowValues_ strings (no
  // vector growth) rather than building a new items/values vector on every
  // render.
  const auto& settings = *currentSettings;
  for (size_t i = 0; i < settings.size(); i++) {
    rowValues_[i] = settingValueText(settings[i]);
    rowItems_[i].value = rowValues_[i].empty() ? nullptr : rowValues_[i].c_str();
  }

  fui::ListProps props;
  props.items = rowItems_.data();
  props.count = static_cast<uint16_t>(rowItems_.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  props.valueInset = 8;               // air between the value and the row edge
  // Titles match the value's font size (smallText) so both sides of a row
  // read as one unit; labels that still don't fit wrap onto a second line.
  // maxLines=2 also marks the style explicitly set (an all-default smallText
  // fails textStyleUnset and the list would substitute bodyText back); the
  // common fits-on-one-line case takes the renderer's fast path anyway.
  props.labelText = screen.theme().bodyText;
  props.labelText.maxLines = 2;
  syncTabListViewport(screen, props);
  screen.list(props);
}

void SettingsActivity::render(RenderLock&&) {
  if (optionPopup.processRender(renderer, mappedInput)) return;

  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto& metrics = UITheme::getInstance().getMetrics();

  // Header via GUI.drawHeader (already FreeInkUI-themed) for the battery
  // indicator; the rest of the screen renders through the app.
  // Version rides in the header's trailing label slot: the footer position
  // conflicts with button hints on non-touch devices.
  drawPageHeader(Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_SETTINGS_TITLE),
                 CROSSPOINT_VERSION);

  renderUi();

  if (usesAccordion()) {
    const auto labels = mainTabButtonLabels(tr(STR_BACK), tr(STR_TOGGLE), listCount() > 1);
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer();
    return;
  }

  const int ring = ringPos();
  const auto confirmLabel =
      usesGridLayout()
          ? tr(STR_SELECT)
          : (ring == 0) ? I18N.get(categoryNames[(selectedCategoryIndex + 1) % categoryCount])
                        : (ring > 0 && (*currentSettings)[ring - 1].nameId == StrId::STR_TIME_TO_SLEEP
                               ? tr(STR_SELECT)
                               : tr(STR_TOGGLE));

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  // Always use standard refresh for settings screen
  renderer.displayBuffer();
}
