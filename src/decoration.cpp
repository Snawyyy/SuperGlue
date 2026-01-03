#include "decoration.hpp"
#include "overlay-state.hpp"
#include "texture-cache.hpp"
#include "pass-element.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>

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
  bool hasAnchor = false;

  // Render volume overlays first (bottom layer)
  for (const auto& info : states) {
    if (info.type == OverlayType::MUTE || info.type == OverlayType::SCROLL_ANCHOR) continue;
    renderOverlay(info, windowBox, a);
  }

  // Render mute overlay (top layer)
  for (const auto& info : states) {
    if (info.type == OverlayType::MUTE) {
      renderOverlay(info, windowBox, a);
    }
  }

  // Render Scroll Anchor + Line
  for (const auto& info : states) {
    if (info.type == OverlayType::SCROLL_ANCHOR) {
      renderAnchorLine(info, windowBox, a);
      hasAnchor = true;
    }
  }

  // Continuous update if anchor is active
  if (hasAnchor) {
    damageEntire();
  }
}

void Superglue::renderAnchorLine(const OverlayInfo& info, const CBox& windowBox, float alpha) {
  // 1. Calculate geometry
  Vector2D anchorPos = {info.x, info.y}; // Anchor center
  Vector2D mousePos = g_pInputManager->getMouseCoordsInternal();

  Vector2D diff = mousePos - anchorPos;
  float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
  
  // Normalized direction
  Vector2D dir = {0, 0};
  if (len > 0.1f) {
    dir = diff / len;
  }

  // 2. Determine Anchor Icon based on direction
  std::string iconName = "anchor.png";
  if (len > 10.0f) {
      if (diff.y > 0) iconName = "anchor_down.png";
      else iconName = "anchor_up.png";
  }
  std::string iconPath = std::string(getenv("HOME")) + "/.icons/" + iconName;

  // 3. Render Dotted Line (Red) - UNDER Anchor
  // Draw dots every 15 pixels
  float step = 15.0f;
  
  // Dynamic Thickness: Standard 4.0, reduces slowly.
  // Formula: 4.0 - (len / 300.0). reaching min size at ~600px.
  float dotSize = 4.0f - (len / 300.0f);
  if (dotSize < 2.0f) dotSize = 2.0f; // Minimum thickness (more firm)

  CHyprColor dotColor;
  dotColor.r = 1.0f;
  dotColor.g = 0.0f;
  dotColor.b = 0.0f;
  dotColor.a = alpha * 0.8f;

  for (float d = step; d < len; d += step) {
    Vector2D dotPos = anchorPos + (dir * d);
    
    // Center dot
    CBox dotBox = {
        dotPos.x - dotSize/2.0,
        dotPos.y - dotSize/2.0,
        (double)dotSize,
        (double)dotSize
    };

    CHyprOpenGLImpl::SRectRenderData rectData;
    rectData.round = 1;
    
    g_pHyprOpenGL->renderRect(dotBox, dotColor, rectData);
  }

  // 4. Render Anchor Icon - ON TOP
  auto& cache = TextureCache::get();
  auto tex = cache.load(iconPath);
  if (tex) {
      Vector2D iconSize = cache.getSize(iconPath);
      CBox iconBox = {
          anchorPos.x - (iconSize.x / 2.0f),
          anchorPos.y - (iconSize.y / 2.0f),
          iconSize.x,
          iconSize.y
      };
      CHyprOpenGLImpl::STextureRenderData texData;
      texData.a = alpha;
      g_pHyprOpenGL->renderTexture(tex, iconBox, texData);
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
  Vector2D pos;

  if (info.hasCustomPos) {
    // Custom position (Global coordinates, e.g. Scroll Anchor)
    // Center icon on the coordinate
    pos.x = info.x - (iconSize.x / 2.0f);
    pos.y = info.y - (iconSize.y / 2.0f);
  } else {
    // Standard position logic
    // Retrieves default config for type (could be optimized)
    auto cfg = config::getDefaultConfig(info.type);
    pos = calculateIconPosition(windowBox, iconSize, cfg.position);
  }

  CBox iconBox = {
      pos.x,
      pos.y,
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
