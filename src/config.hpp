#pragma once

#include <string>
#include "types.hpp"

/**
 * Default configuration values.
 */
namespace config {

// Timing
constexpr int DEFAULT_DISPLAY_MS = 800;
constexpr int DEFAULT_FADE_MS = 100;
constexpr int DEFAULT_POLL_INTERVAL_MS = 10;

// Sizing
constexpr int DEFAULT_ICON_SIZE = 128;
constexpr int DEFAULT_PADDING = 10;
constexpr int MAX_STACKED_EVENTS = 3;

// IPC file paths
inline const std::string MUTE_STATE_FILE = "/tmp/volume-mute-state";
inline const std::string OVERLAY_CMD_FILE = "/tmp/superglue-overlay-cmd";
inline const std::string LOG_FILE = "/tmp/superglue.log";

/**
 * Returns the default icon path for an overlay type.
 */
inline std::string getDefaultIconPath(OverlayType type) {
  std::string home = getenv("HOME");
  switch (type) {
    case OverlayType::VOLUME_UP:
      return home + "/.icons/up.png";
    case OverlayType::VOLUME_DOWN:
      return home + "/.icons/down.png";
    case OverlayType::MUTE:
      return home + "/.icons/mute.png";
    case OverlayType::SCROLL_ANCHOR:
      return home + "/.icons/anchor.png";
    default:
      return "";
  }
}

/**
 * Returns the volume level icon path based on percentage.
 * Maps 0-100% to volume_0.png through volume_13.png.
 */
inline std::string getVolumeLevelIconPath(int volumePercent) {
  std::string home = getenv("HOME");
  // Clamp to 0-100
  if (volumePercent < 0) volumePercent = 0;
  if (volumePercent > 100) volumePercent = 100;
  // Map 0-100 to 0-13 (14 icons total)
  int iconIndex = (volumePercent * 13) / 100;
  return home + "/.icons/volume_" + std::to_string(iconIndex) + ".png";
}

/**
 * Returns the default config for an overlay type.
 */
inline OverlayConfig getDefaultConfig(OverlayType type) {
  OverlayConfig cfg;
  cfg.iconPath = getDefaultIconPath(type);
  cfg.position = Position::CENTER;
  cfg.displayMs = DEFAULT_DISPLAY_MS;
  cfg.fadeMs = DEFAULT_FADE_MS;
  cfg.iconSize = DEFAULT_ICON_SIZE;
  cfg.padding = DEFAULT_PADDING;
  return cfg;
}

}  // namespace config
