#pragma once

#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include "types.hpp"
#include "config.hpp"
#include <vector>

/**
 * Window decoration that displays overlay icons.
 * Supports configurable positions and multiple overlay types.
 */
class Superglue : public IHyprWindowDecoration {
 public:
  explicit Superglue(PHLWINDOW pWindow);
  virtual ~Superglue();

  virtual void draw(PHLMONITOR pMonitor, float const& a) override;
  virtual SDecorationPositioningInfo getPositioningInfo() override;
  virtual void onPositioningReply(
      const SDecorationPositioningReply& reply) override;
  virtual eDecorationType getDecorationType() override;
  virtual eDecorationLayer getDecorationLayer() override;
  virtual uint64_t getDecorationFlags() override;
  virtual std::string getDisplayName() override;
  virtual void updateWindow(PHLWINDOW pWindow) override;
  virtual void damageEntire() override;

  void renderPass(PHLMONITOR pMonitor, float a);
  CBox assignedBoxGlobal();
  std::string getWindowAddress() { return m_windowAddress; }
  CBox getVisualBox();

 private:
  Vector2D calculateIconPosition(
      const CBox& windowBox,
      const Vector2D& iconSize,
      Position position);

  void renderOverlay(
      const OverlayInfo& info,
      const CBox& windowBox,
      float alpha,
      const Vector2D& monitorPos);

  void renderAnchorLine(
      const OverlayInfo& info,
      const CBox& windowBox,
      float alpha,
      const Vector2D& monitorPos);

  PHLWINDOWREF m_pWindowRef;
  std::string m_windowAddress;
  CBox m_bAssignedBox;
};
