#include "OverlayState.hpp"
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/Compositor.hpp>
#include "Superglue.hpp"

// Do NOT include the definition here, it's in OverlayState.cpp
// extern std::unique_ptr<OverlayState> g_pOverlayState; 

#include "OverlayState.hpp"
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/Compositor.hpp>
#include "Superglue.hpp"

HANDLE PHANDLE = nullptr;

// Helper to add decoration
void addDecoration(PHLWINDOW pWindow) {
    if (!pWindow) return;
    
    // Check if already has it
    // HyprlandAPI::addWindowDecoration handles uniqueness usually or returns false?
    // Let's rely on our own check if needed, but for now just try adding.
    
    OverlayState::get()->log("Adding decoration to window: " + std::format("0x{:x}", (uintptr_t)pWindow.get()));
    
    // We need to create the decoration and add it to the window
    auto deco = makeUnique<Superglue>(pWindow);
    HyprlandAPI::addWindowDecoration(PHANDLE, pWindow, std::move(deco));
}

void onNewWindow(void* self, std::any data) {
    auto pWindow = std::any_cast<PHLWINDOW>(data);
    addDecoration(pWindow);
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    try {
        // Initialize global state
        fprintf(stderr, "[SUPERGLUE] Initializing global state...\n");
        g_pOverlayState = std::make_unique<OverlayState>();
        g_pOverlayState->init(); 
        
        fprintf(stderr, "[SUPERGLUE] Global state initialized.\n");
        
        // Register callbacks
        static auto P = HyprlandAPI::registerCallbackDynamic(PHANDLE, "openWindow", [&](void* self, SCallbackInfo& info, std::any data) { onNewWindow(self, data); });
        
        fprintf(stderr, "[SUPERGLUE] Callback registered. Iterating windows...\n");
        
        if (!g_pCompositor) {
             fprintf(stderr, "[SUPERGLUE] FATAL - g_pCompositor is null!\n");
             return {"Superglue", "Sticks icons to windows", "Snawy", "1.0"};
        }
        
        fprintf(stderr, "[SUPERGLUE] Window count: %zu\n", g_pCompositor->m_windows.size());

        int count = 0;
        // Add to existing windows
        for (auto& w : g_pCompositor->m_windows) {
            fprintf(stderr, "[SUPERGLUE] Checking window: 0x%lx Mapped: %d Hidden: %d\n", (uintptr_t)w.get(), w->m_isMapped, w->isHidden());
            if (w->m_isMapped && !w->isHidden()) {
                 addDecoration(w);
                 count++;
            }
        }
        fprintf(stderr, "[SUPERGLUE] Added decoration to %d windows.\n", count);
    } catch (const std::exception& e) {
        fprintf(stderr, "[SUPERGLUE] Exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "[SUPERGLUE] Unknown Exception\n");
    }

    return {"Superglue", "Sticks icons to windows", "Snawy", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    // Clean up
    g_pOverlayState.reset();
}
