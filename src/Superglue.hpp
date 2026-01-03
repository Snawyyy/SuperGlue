#pragma once

#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include "OverlayState.hpp"

class Superglue : public IHyprWindowDecoration {
public:
    Superglue(PHLWINDOW pWindow);
    virtual ~Superglue();

    virtual void draw(PHLMONITOR pMonitor, float const& a) override;

    void renderPass(PHLMONITOR pMonitor, float a);

    // Helper for bounding box
    CBox assignedBoxGlobal();

    virtual SDecorationPositioningInfo getPositioningInfo() override;
    virtual void onPositioningReply(const SDecorationPositioningReply& reply) override;
    virtual eDecorationType getDecorationType() override;
    virtual eDecorationLayer getDecorationLayer() override;
    virtual uint64_t getDecorationFlags() override;
    virtual std::string getDisplayName() override;
    virtual void updateWindow(PHLWINDOW pWindow) override;
    virtual void damageEntire() override;
    
    // Updated simple logic
    OverlayInfo getOverlayState();
    
    std::string getWindowAddress() { return m_windowAddress; }

private:
    PHLWINDOWREF m_pWindowRef;
    std::string m_windowAddress;
    
    // Cache the textures
    SP<CTexture> m_texMute;
    SP<CTexture> m_texVolUp;
    SP<CTexture> m_texVolDown;
    
    Vector2D m_vIconSize = {128, 128}; 
    CBox m_bAssignedBox;
    
    // Helpers
    std::string getIconPath(OverlayType type);
    SP<CTexture> loadTexture(OverlayType type);
};
