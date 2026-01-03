#include "decoration.hpp"
#include "overlay-state.hpp"
#include "texture-cache.hpp"
#include "pass-element.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>

Superglue::Superglue(PHLWINDOW pWindow)
    : IHyprWindowDecoration(pWindow) {
  m_pWindowRef = pWindow;
  m_windowAddress = std::format("0x{:x}", (uintptr_t)pWindow.get());

  if (OverlayState::get()) {
    OverlayState::get()->registerWindow(this);
  }
}

Superglue::~Superglue() {
  if (OverlayState::get()) {
    OverlayState::get()->unregisterWindow(this);
  }
}

SDecorationPositioningInfo Superglue::getPositioningInfo() {
  SDecorationPositioningInfo info;
  info.policy = DECORATION_POSITION_ABSOLUTE;
  info.priority = 10000;
  return info;
}

void Superglue::onPositioningReply(
    const SDecorationPositioningReply& reply) {
  m_bAssignedBox = reply.assignedGeometry;
}

eDecorationType Superglue::getDecorationType() {
  return DECORATION_CUSTOM;
}

eDecorationLayer Superglue::getDecorationLayer() {
  return DECORATION_LAYER_OVERLAY;
}

uint64_t Superglue::getDecorationFlags() {
  return DECORATION_PART_OF_MAIN_WINDOW;
}

std::string Superglue::getDisplayName() {
  return "Superglue";
}

void Superglue::updateWindow(PHLWINDOW pWindow) {
  damageEntire();
}

void Superglue::damageEntire() {
  auto pWindow = m_pWindowRef.lock();
  if (!pWindow) return;

  Vector2D pos = pWindow->m_realPosition->value();
  Vector2D size = pWindow->m_realSize->value();
  CBox box = {pos.x, pos.y, size.x, size.y};
  g_pHyprRenderer->damageBox(box);
}

CBox Superglue::assignedBoxGlobal() {
  auto pWindow = m_pWindowRef.lock();
  if (!pWindow) return {};

  CBox box;
  box.x = pWindow->m_realPosition->value().x;
  box.y = pWindow->m_realPosition->value().y;
  box.w = pWindow->m_realSize->value().x;
  box.h = pWindow->m_realSize->value().y;
  return box;
}

void Superglue::draw(PHLMONITOR pMonitor, float const& a) {
  auto pWindow = m_pWindowRef.lock();
  if (!pWindow || !pWindow->m_isMapped || pWindow->isHidden()) {
    return;
  }

  if (!OverlayState::get()) return;

  auto states = OverlayState::get()->getOverlayInfo(m_windowAddress);
  if (states.empty()) return;

  GluePassElement::SGlueData data;
  data.deco = this;
  data.a = a;
  g_pHyprRenderer->m_renderPass.add(makeUnique<GluePassElement>(data));
}

void Superglue::renderPass(PHLMONITOR pMonitor, float a) {
  auto pWindow = m_pWindowRef.lock();
  if (!pWindow || !pWindow->m_isMapped || pWindow->isHidden()) {
    return;
  }

  if (!OverlayState::get()) return;

  auto states = OverlayState::get()->getOverlayInfo(m_windowAddress);
  if (states.empty()) return;

  CBox windowBox = assignedBoxGlobal();

  // Render volume overlays first (bottom layer)
  for (const auto& info : states) {
    if (info.type == OverlayType::MUTE) continue;
    renderOverlay(info, windowBox, a);
  }

  // Render mute overlay last (top layer)
  for (const auto& info : states) {
    if (info.type != OverlayType::MUTE) continue;
    renderOverlay(info, windowBox, a);
  }
}

void Superglue::renderOverlay(
    const OverlayInfo& info,
    const CBox& windowBox,
    float alpha) {
  if (info.iconPath.empty()) return;

  auto& cache = TextureCache::get();
  auto tex = cache.load(info.iconPath);
  if (!tex) return;

  Vector2D iconSize = cache.getSize(info.iconPath);
  Vector2D pos = calculateIconPosition(
      windowBox, iconSize, Position::CENTER);

  CBox iconBox = {
      std::round(pos.x),
      std::round(pos.y),
      iconSize.x,
      iconSize.y
  };

  CHyprOpenGLImpl::STextureRenderData texData;
  texData.a = alpha * info.opacity;
  g_pHyprOpenGL->renderTexture(tex, iconBox, texData);
}

Vector2D Superglue::calculateIconPosition(
    const CBox& windowBox,
    const Vector2D& iconSize,
    Position position) {
  float x = 0, y = 0;
  float pad = config::DEFAULT_PADDING;

  switch (position) {
    case Position::TOP_LEFT:
      x = windowBox.x + pad;
      y = windowBox.y + pad;
      break;
    case Position::TOP_RIGHT:
      x = windowBox.x + windowBox.w - iconSize.x - pad;
      y = windowBox.y + pad;
      break;
    case Position::BOTTOM_LEFT:
      x = windowBox.x + pad;
      y = windowBox.y + windowBox.h - iconSize.y - pad;
      break;
    case Position::BOTTOM_RIGHT:
      x = windowBox.x + windowBox.w - iconSize.x - pad;
      y = windowBox.y + windowBox.h - iconSize.y - pad;
      break;
    case Position::TOP_CENTER:
      x = windowBox.x + (windowBox.w - iconSize.x) / 2;
      y = windowBox.y + pad;
      break;
    case Position::BOTTOM_CENTER:
      x = windowBox.x + (windowBox.w - iconSize.x) / 2;
      y = windowBox.y + windowBox.h - iconSize.y - pad;
      break;
    case Position::CENTER:
    default:
      x = windowBox.x + (windowBox.w - iconSize.x) / 2;
      y = windowBox.y + (windowBox.h - iconSize.y) / 2;
      break;
  }

  return {x, y};
}
