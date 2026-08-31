#include "QuicksandActivity.h"

#include <Arduino.h>
#include <math.h>

#include "components/UITheme.h"
#include "fontIds.h"

namespace {

constexpr unsigned long kFrameMs = 300;  // throttle refresh to protect the e-ink panel

}  // namespace

void QuicksandActivity::onEnter() {
  Activity::onEnter();
  startMs_ = millis();
  lastFrameMs_ = startMs_;
  requestUpdate();
}

void QuicksandActivity::addRippleAt(float nx, float ny) {
  const int idx = rippleN_ < 4 ? rippleN_ : 0;
  rippleXs_[idx] = nx;
  rippleYs_[idx] = ny;
  if (rippleN_ < 4) ++rippleN_;
  // The ripple repels the drifting driver so the flow "dodges" the touch.
  driverVx_ += (nx - driverX_) * 0.003f;
  driverVy_ += (ny - driverY_) * 0.003f;
}

void QuicksandActivity::loop() {
  const unsigned long now = millis();
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goToApps();
    return;
  }
  int tx = 0;
  int ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty) || mappedInput.wasScreenTouchDown(tx, ty)) {
    const float nx = static_cast<float>(tx) / static_cast<float>(renderer.getScreenWidth());
    const float ny = static_cast<float>(ty) / static_cast<float>(renderer.getScreenHeight());
    addRippleAt(nx, ny);
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
             mappedInput.wasReleased(MappedInputManager::Button::Down) ||
             mappedInput.wasReleased(MappedInputManager::Button::Left) ||
             mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    // Button-only boards (EEGO A4) drop a ripple at the current driver center.
    addRippleAt(driverX_, driverY_);
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    addRippleAt(driverX_, driverY_);
  }

  // Refresh on a throttle; the scene always moves, so keep animating.
  if (now - lastFrameMs_ >= kFrameMs) {
    lastFrameMs_ = now;
    requestUpdate();
  }
}

void QuicksandActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  const float t = static_cast<float>(millis() - startMs_) / 1000.0f;

  // Drift the source along a gentle sine path, then apply the driver "flow".
  driverX_ += driverVx_;
  driverY_ += driverVy_;
  driverX_ += 0.0015f * sinf(t * 0.4f);
  driverY_ += 0.0012f * cosf(t * 0.3f);
  if (driverX_ < 0.15f) { driverX_ = 0.15f; driverVx_ = -driverVx_; }
  if (driverX_ > 0.85f) { driverX_ = 0.85f; driverVx_ = -driverVx_; }
  if (driverY_ < 0.15f) { driverY_ = 0.15f; driverVy_ = -driverVy_; }
  if (driverY_ > 0.85f) { driverY_ = 0.85f; driverVy_ = -driverVy_; }

  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_QUICKSAND_TITLE));

  const int cx = static_cast<int>(driverX_ * pageWidth);
  const int cy = static_cast<int>(driverY_ * (pageHeight - metrics.headerHeight)) + metrics.headerHeight;
  const int maxRadius = pageHeight;

  // Concentric, slowly breathing rings centered on the drifting source.
  const int ringCount = 6;
  for (int i = 0; i < ringCount; ++i) {
    const float phase = t * 0.3f;
    const float radius = (static_cast<float>(i + 1) / static_cast<float>(ringCount)) * maxRadius;
    const float wobble = 6.0f * sinf(t * 0.7f + static_cast<float>(i));
    const float r = radius + wobble * (static_cast<float>(i % 2 == 0 ? 1 : -1));
    renderer.drawArc(static_cast<int>(r), cx, cy, 1, 1, 1, true);
    renderer.drawArc(static_cast<int>(r), cx, cy, -1, 1, 1, true);
    renderer.drawArc(static_cast<int>(r), cx, cy, 1, -1, 1, true);
    renderer.drawArc(static_cast<int>(r), cx, cy, -1, -1, 1, true);
  }

  // Expanding "avoidance" ripples from recent touches.
  for (int i = 0; i < rippleN_; ++i) {
    if (rippleXs_[i] < 0.f) continue;
    const int rx = static_cast<int>(rippleXs_[i] * pageWidth);
    const int ry = static_cast<int>(rippleYs_[i] * (pageHeight - metrics.headerHeight)) + metrics.headerHeight;
    for (int ring = 0; ring < 3; ++ring) {
      const float rad = 18.0f + static_cast<float>(ring) * 14.0f;
      renderer.drawArc(static_cast<int>(rad), rx, ry, 1, 1, 1, false);
      renderer.drawArc(static_cast<int>(rad), rx, ry, -1, 1, 1, false);
      renderer.drawArc(static_cast<int>(rad), rx, ry, 1, -1, 1, false);
      renderer.drawArc(static_cast<int>(rad), rx, ry, -1, -1, 1, false);
    }
  }

  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - metrics.buttonHintsHeight - metrics.verticalSpacing,
                            tr(STR_QUICKSAND_HINT), true, EpdFontFamily::REGULAR);
  renderer.displayBuffer(HalDisplay::RefreshMode::HALF_REFRESH);
}