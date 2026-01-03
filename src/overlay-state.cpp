#include "overlay-state.hpp"
#include "file-watcher.hpp"
#include "decoration.hpp"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <hyprland/src/Compositor.hpp>

std::unique_ptr<OverlayState> g_pOverlayState;

OverlayState* OverlayState::get() {
  return g_pOverlayState.get();
}

static int handleEvent(int fd, uint32_t mask, void* data) {
  return ((OverlayState*)data)->onEvent(fd, mask);
}

OverlayState::OverlayState() {
  log("OverlayState constructor");
  if (pipe(m_eventFd) != 0) {
    log("Failed to create pipe!");
  }
  initFileWatcher();
}

OverlayState::~OverlayState() {
  log("OverlayState destructor");
  shutdown();
  if (m_eventSource) wl_event_source_remove(m_eventSource);
  close(m_eventFd[0]);
  close(m_eventFd[1]);
}

void OverlayState::initFileWatcher() {
  m_watcher = std::make_unique<FileWatcher>(
      config::DEFAULT_POLL_INTERVAL_MS);

  m_watcher->watch(
      config::MUTE_STATE_FILE,
      [this](const std::string& content) {
        onMuteStateChanged(content);
      });

  m_watcher->watch(
      config::OVERLAY_CMD_FILE,
      [this](const std::string& content) {
        onOverlayCommand(content);
      });
}

void OverlayState::init() {
  if (g_pCompositor && g_pCompositor->m_wlDisplay) {
    wl_event_loop* loop =
        wl_display_get_event_loop(g_pCompositor->m_wlDisplay);
    m_eventSource = wl_event_loop_add_fd(
        loop, m_eventFd[0], WL_EVENT_READABLE, handleEvent, this);
    log("Event loop hook registered.");
  } else {
    log("FATAL: Compositor not available during init!");
  }
}

int OverlayState::onEvent(int fd, uint32_t mask) {
  if (mask & WL_EVENT_READABLE) {
    char buf[8];
    read(fd, buf, sizeof(buf));

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto* win : m_windows) {
      if (win) win->damageEntire();
    }
  }
  return 0;
}

void OverlayState::dispatchDamage() {
  char c = 1;
  if (write(m_eventFd[1], &c, 1) < 0) {
    log("Failed to write to pipe!");
  }
}

void OverlayState::shutdown() {
  if (m_watcher) m_watcher->stop();
}

void OverlayState::log(const std::string& msg) {
  static std::mutex logMutex;
  std::lock_guard<std::mutex> lock(logMutex);
  std::ofstream f(config::LOG_FILE, std::ios::app);
  f << msg << std::endl;
}

void OverlayState::registerWindow(Superglue* win) {
  log("Registering window: " + win->getWindowAddress());
  std::lock_guard<std::mutex> lock(m_mutex);
  m_windows.insert(win);
}

void OverlayState::unregisterWindow(Superglue* win) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_windows.erase(win);
}

void OverlayState::onMuteStateChanged(const std::string& content) {
  std::unordered_set<std::string> newAddresses;
  std::istringstream stream(content);
  std::string line;

  while (std::getline(stream, line)) {
    if (!line.empty()) newAddresses.insert(line);
  }

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mutedAddresses = newAddresses;
  }
  dispatchDamage();
}

void OverlayState::onOverlayCommand(const std::string& content) {
  if (content.empty()) return;

  std::istringstream stream(content);
  std::string line;
  bool damageNeeded = false;

  while (std::getline(stream, line)) {
    if (line.empty()) continue;

    std::istringstream lineStream(line);
    std::string cmd, addr;
    int volume = 0;
    lineStream >> cmd >> addr >> volume;

    if (addr.empty()) continue;

    log("Received cmd: " + cmd + " for " + addr +
        " vol: " + std::to_string(volume));

    std::lock_guard<std::mutex> lock(m_mutex);

    // Clear previous events for this address to avoid stacking
    m_volumeEvents[addr].clear();

    auto now = std::chrono::steady_clock::now();

    // Create volume level event (shows in center)
    OverlayEvent levelEvent;
    levelEvent.type = OverlayType::VOLUME_LEVEL;
    levelEvent.startTime = now;
    levelEvent.volumeLevel = volume;
    m_volumeEvents[addr].push_back(levelEvent);

    // Create direction arrow event
    OverlayEvent arrowEvent;
    arrowEvent.startTime = now;
    arrowEvent.volumeLevel = volume;
    if (cmd == "vol-up") {
      arrowEvent.type = OverlayType::VOLUME_UP;
    } else if (cmd == "vol-down") {
      arrowEvent.type = OverlayType::VOLUME_DOWN;
    } else {
      continue;
    }
    m_volumeEvents[addr].push_back(arrowEvent);

    damageNeeded = true;
  }

  // Clear the command file
  std::ofstream clear(config::OVERLAY_CMD_FILE, std::ios::trunc);

  if (damageNeeded) dispatchDamage();
}

float OverlayState::calculateOpacity(const OverlayEvent& event) {
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - event.startTime).count();

  if (elapsed < config::DEFAULT_DISPLAY_MS) {
    return 1.0f;
  }

  int fadeEnd = config::DEFAULT_DISPLAY_MS + config::DEFAULT_FADE_MS;
  if (elapsed < fadeEnd) {
    return 1.0f - (float)(elapsed - config::DEFAULT_DISPLAY_MS) /
        config::DEFAULT_FADE_MS;
  }
  return 0.0f;
}

std::vector<OverlayInfo> OverlayState::getOverlayInfo(
    const std::string& address) {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<OverlayInfo> result;

  // Volume events
  if (m_volumeEvents.contains(address)) {
    auto& events = m_volumeEvents[address];
    for (auto it = events.begin(); it != events.end();) {
      float opacity = calculateOpacity(*it);
      if (opacity <= 0.0f) {
        it = events.erase(it);
      } else {
        OverlayInfo info;
        info.type = it->type;
        info.opacity = opacity;
        info.volumeLevel = it->volumeLevel;

        // Use volume level icon for VOLUME_LEVEL type
        if (it->type == OverlayType::VOLUME_LEVEL) {
          info.iconPath = config::getVolumeLevelIconPath(it->volumeLevel);
        } else {
          info.iconPath = config::getDefaultIconPath(it->type);
        }

        result.push_back(info);
        ++it;
      }
    }
  }

  // Mute state
  if (m_mutedAddresses.contains(address)) {
    OverlayInfo info;
    info.type = OverlayType::MUTE;
    info.opacity = 1.0f;
    info.iconPath = config::getDefaultIconPath(OverlayType::MUTE);
    result.push_back(info);
  }

  return result;
}
