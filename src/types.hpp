#pragma once

#include <string>
#include <chrono>
#include <vector>

/**
 * Overlay position on the window.
 */
enum class Position {
  CENTER,
  TOP_LEFT,
  TOP_RIGHT,
  BOTTOM_LEFT,
  BOTTOM_RIGHT,
  TOP_CENTER,
  BOTTOM_CENTER
};

/**
 * Type of overlay to display.
 */
enum class OverlayType {
  NONE,
  MUTE,
  VOLUME_UP,
  VOLUME_DOWN,
  VOLUME_LEVEL,
  SCROLL_ANCHOR
};

/**
 * Configuration for a single overlay type.
 */
struct OverlayConfig {
  std::string iconPath;
  Position position = Position::CENTER;
  int displayMs = 800;
  int fadeMs = 100;
  int iconSize = 128;
  int padding = 10;
};

/**
 * Transient overlay event with timestamp.
 */
struct OverlayEvent {
  OverlayType type;
  std::chrono::steady_clock::time_point startTime;
  int volumeLevel = 0;  // 0-100 percentage
  // For scroll anchor (global coordinates)
  double x = 0;
  double y = 0;
};

/**
 * Computed overlay info for rendering.
 */
struct OverlayInfo {
  OverlayType type = OverlayType::NONE;
  float opacity = 0.0f;
  std::string iconPath;
  int volumeLevel = 0;  // 0-100 percentage
  // Position override (if set, ignores standard layout)
  bool hasCustomPos = false;
  double x = 0;
  double y = 0;
};

/**
 * Parses position string to enum.
 */
inline Position parsePosition(const std::string& str) {
  if (str == "top-left") return Position::TOP_LEFT;
  if (str == "top-right") return Position::TOP_RIGHT;
  if (str == "bottom-left") return Position::BOTTOM_LEFT;
  if (str == "bottom-right") return Position::BOTTOM_RIGHT;
  if (str == "top-center") return Position::TOP_CENTER;
  if (str == "bottom-center") return Position::BOTTOM_CENTER;
  return Position::CENTER;
}

/**
 * Converts position enum to string.
 */
inline std::string positionToString(Position pos) {
  switch (pos) {
    case Position::TOP_LEFT: return "top-left";
    case Position::TOP_RIGHT: return "top-right";
    case Position::BOTTOM_LEFT: return "bottom-left";
    case Position::BOTTOM_RIGHT: return "bottom-right";
    case Position::TOP_CENTER: return "top-center";
    case Position::BOTTOM_CENTER: return "bottom-center";
    default: return "center";
  }
}
