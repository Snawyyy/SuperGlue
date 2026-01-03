#include "overlay-state.hpp"
#include "decoration.hpp"
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/Compositor.hpp>

HANDLE PHANDLE = nullptr;

static void addDecoration(PHLWINDOW pWindow) {
  if (!pWindow) return;

  if (OverlayState::get()) {
    OverlayState::get()->log(
        "Adding decoration to: " +
        std::format("0x{:x}", (uintptr_t)pWindow.get()));
  }

  auto deco = makeUnique<Superglue>(pWindow);
  HyprlandAPI::addWindowDecoration(PHANDLE, pWindow, std::move(deco));
}

static void onNewWindow(void* self, std::any data) {
  auto pWindow = std::any_cast<PHLWINDOW>(data);
  addDecoration(pWindow);
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
  return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  try {
    fprintf(stderr, "[SUPERGLUE] Initializing...\n");

    g_pOverlayState = std::make_unique<OverlayState>();
    g_pOverlayState->init();

    static auto P = HyprlandAPI::registerCallbackDynamic(
        PHANDLE, "openWindow",
        [&](void* self, SCallbackInfo& info, std::any data) {
          onNewWindow(self, data);
        });

    if (!g_pCompositor) {
      fprintf(stderr, "[SUPERGLUE] FATAL - g_pCompositor is null!\n");
      return {"Superglue", "Overlay decorations", "Snawy", "2.0"};
    }

    int count = 0;
    for (auto& w : g_pCompositor->m_windows) {
      if (w->m_isMapped && !w->isHidden()) {
        addDecoration(w);
        count++;
      }
    }

    fprintf(stderr, "[SUPERGLUE] Added to %d windows.\n", count);

  } catch (const std::exception& e) {
    fprintf(stderr, "[SUPERGLUE] Exception: %s\n", e.what());
  } catch (...) {
    fprintf(stderr, "[SUPERGLUE] Unknown exception\n");
  }

  return {"Superglue", "Overlay decorations", "Snawy", "2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
  g_pOverlayState.reset();
}
