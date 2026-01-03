#include "Superglue.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <cairo/cairo.h>
#include "GluePassElement.hpp"

// Helpers to load png
SP<CTexture> Superglue::loadTexture(OverlayType type) {
    std::string path = getIconPath(type);
    if (OverlayState::get()) OverlayState::get()->log("Loading texture from: " + path);
    
    cairo_surface_t* surface = cairo_image_surface_create_from_png(path.c_str());
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        if (OverlayState::get()) OverlayState::get()->log("Failed to load texture: " + path + " Status: " + std::to_string(cairo_surface_status(surface)));
        cairo_surface_destroy(surface);
        return nullptr;
    }

    int w = cairo_image_surface_get_width(surface);
    int h = cairo_image_surface_get_height(surface);
    const auto DATA = cairo_image_surface_get_data(surface);
    
    if (OverlayState::get()) OverlayState::get()->log("Texture loaded: " + std::to_string(w) + "x" + std::to_string(h));

    // Update icon size reference
    m_vIconSize = {(double)w, (double)h};

    SP<CTexture> tex = makeShared<CTexture>();
    tex->allocate();
    glBindTexture(GL_TEXTURE_2D, tex->m_texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
#ifndef GLES2
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);
    
    cairo_surface_destroy(surface);
    return tex;
}

Superglue::Superglue(PHLWINDOW pWindow) : IHyprWindowDecoration(pWindow) {
    m_pWindowRef = pWindow;
    m_windowAddress = std::format("0x{:x}", (uintptr_t)pWindow.get());
    
    // Register with OverlayState
    if (OverlayState::get()) OverlayState::get()->registerWindow(this);
}

Superglue::~Superglue() {
    if (OverlayState::get()) OverlayState::get()->unregisterWindow(this);
}

SDecorationPositioningInfo Superglue::getPositioningInfo() {
    SDecorationPositioningInfo info;
    info.policy = DECORATION_POSITION_ABSOLUTE; // We manage our own geometry
    info.priority = 10000; // High priority to ensure it's on top
    return info;
}

void Superglue::onPositioningReply(const SDecorationPositioningReply& reply) {
    m_bAssignedBox = reply.assignedGeometry;
}

void Superglue::draw(PHLMONITOR pMonitor, float const& a) { 
    auto pWindow = m_pWindowRef.lock();
    if (!pWindow || !pWindow->m_isMapped || pWindow->isHidden())
        return;

    // Check if we have anything to draw before adding pass element
    if (getOverlayState().type == OverlayType::None)
        return;

    // Add our custom pass element to the renderer
    GluePassElement::SGlueData data;
    data.deco = this;
    data.a = a;
    
    g_pHyprRenderer->m_renderPass.add(makeUnique<GluePassElement>(data));
}

void Superglue::renderPass(PHLMONITOR pMonitor, float a) { 
    auto pWindow = m_pWindowRef.lock();
    if (!pWindow || !pWindow->m_isMapped || pWindow->isHidden())
        return;

    // Check if we have anything to draw
    OverlayInfo state = getOverlayState();
    if (state.type == OverlayType::None)
        return;
    
    // Use proper member access for animated variables
    Vector2D windowPos = pWindow->m_realPosition->value(); 
    Vector2D windowSize = pWindow->m_realSize->value();
    
    // Load Texture if needed
    SP<CTexture> currentTex;
    switch (state.type) {
        case OverlayType::Mute:
             if (!m_texMute) m_texMute = loadTexture(OverlayType::Mute);
             currentTex = m_texMute;
             break;
        case OverlayType::VolumeUp:
             if (!m_texVolUp) m_texVolUp = loadTexture(OverlayType::VolumeUp);
             currentTex = m_texVolUp;
             break;
        case OverlayType::VolumeDown:
             if (!m_texVolDown) m_texVolDown = loadTexture(OverlayType::VolumeDown);
             currentTex = m_texVolDown;
             break;
        default: break;
    }
    
    if (!currentTex) return;

    float iconW = m_vIconSize.x;
    float iconH = m_vIconSize.y;
    
    // Center the icon on the window
    float iconX = std::round(windowPos.x + (windowSize.x / 2.0f) - (iconW / 2.0f));
    float iconY = std::round(windowPos.y + (windowSize.y / 2.0f) - (iconH / 2.0f));

    CBox iconBox = {iconX, iconY, iconW, iconH};

    CHyprOpenGLImpl::STextureRenderData texData;
    // Combine hyprland alpha with overlay state alpha (fade out)
    texData.a = a * state.opacity;
    
    if (currentTex)
        g_pHyprOpenGL->renderTexture(currentTex, iconBox, texData);
}

eDecorationType Superglue::getDecorationType() {
    return DECORATION_CUSTOM;
}

void Superglue::updateWindow(PHLWINDOW pWindow) {
    damageEntire();
}

void Superglue::damageEntire() {
    auto pWindow = m_pWindowRef.lock();
    if (pWindow) {
         Vector2D pos = pWindow->m_realPosition->value();
         Vector2D size = pWindow->m_realSize->value();
         CBox box = {pos.x, pos.y, size.x, size.y};
         g_pHyprRenderer->damageBox(box);
    }
}

eDecorationLayer Superglue::getDecorationLayer() {
    return DECORATION_LAYER_OVERLAY; // Draw on TOP of the window
}

uint64_t Superglue::getDecorationFlags() {
    return DECORATION_PART_OF_MAIN_WINDOW; 
}

std::string Superglue::getDisplayName() {
    return "SuperglueMuteIcon";
}

OverlayInfo Superglue::getOverlayState() {
    if (OverlayState::get()) return OverlayState::get()->getOverlayInfo(m_windowAddress);
    return {OverlayType::None, 0.0f};
}

std::string Superglue::getIconPath(OverlayType type) {
    std::string home = getenv("HOME");
    switch (type) {
        case OverlayType::VolumeUp: return home + "/.icons/up.png";
        case OverlayType::VolumeDown: return home + "/.icons/down.png";
        default: return home + "/.icons/mute.png";
    }
}

CBox Superglue::assignedBoxGlobal() {
    auto pWindow = m_pWindowRef.lock();
    if (pWindow) {
         CBox box;
         box.x = pWindow->m_realPosition->value().x;
         box.y = pWindow->m_realPosition->value().y;
         box.w = pWindow->m_realSize->value().x;
         box.h = pWindow->m_realSize->value().y;
         return box;
    }
    return {};
}
