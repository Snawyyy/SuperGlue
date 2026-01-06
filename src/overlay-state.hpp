#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <wayland-server.h>
#include "types.hpp"
#include "config.hpp"

class Superglue;
class FileWatcher;

/**
 * Manages overlay state for all windows.
 * Tracks muted windows and transient volume events.
 */
class OverlayState {
 public:
  OverlayState();
  ~OverlayState();

  static OverlayState* get();

  /**
   * Gets overlay info for a window by address.
   */
  std::vector<OverlayInfo> getOverlayInfo(const std::string& address);

  /**
   * Registers a window decoration for damage updates.
   */
  void registerWindow(Superglue* win);

  /**
   * Unregisters a window decoration.
   */
  void unregisterWindow(Superglue* win);

  /**
   * Logs a message to the log file.
   */
  void log(const std::string& msg);

  /**
   * Stops background processing.
   */
  void shutdown();

  /**
   * Initializes the Wayland event loop hook.
   */
  void init();

  /**
   * Handles events from the pipe.
   */
  int onEvent(int fd, uint32_t mask);

 private:
  void initFileWatcher();
  void onMuteStateChanged(const std::string& content);
  void onOverlayCommand(const std::string& content);
  
  // Helper methods for command parsing
  void handleScrollCommand(
      std::istringstream& lineStream,
      bool& damageNeeded);
  void handleVolumeCommand(
      std::istringstream& lineStream,
      const std::string& cmd,
      bool& damageNeeded);

  // Helper methods for overlay info
  void appendScrollInfo(
      const std::string& address,
      std::vector<OverlayInfo>& result);
  void appendVolumeInfo(
      const std::string& address,
      std::vector<OverlayInfo>& result);
  void appendMuteInfo(
      const std::string& address,
      std::vector<OverlayInfo>& result);

  void dispatchDamage();
  float calculateOpacity(const OverlayEvent& event);

  std::unordered_map<std::string, std::vector<OverlayEvent>>
      m_volumeEvents;
  std::unordered_map<std::string, OverlayEvent> m_scrollAnchors;
  std::unordered_set<std::string> m_mutedAddresses;
  std::unordered_set<Superglue*> m_windows;

  std::unique_ptr<FileWatcher> m_watcher;
  std::recursive_mutex m_mutex;

  int m_eventFd[2];
  wl_event_source* m_eventSource = nullptr;
};

extern std::unique_ptr<OverlayState> g_pOverlayState;
