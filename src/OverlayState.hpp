#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <mutex>
#include <wayland-server.h>

class Superglue;

enum class OverlayType {
    None,
    Mute,
    VolumeUp,
    VolumeDown
};

struct OverlayInfo {
    OverlayType type = OverlayType::None;
    float opacity = 0.0f;
};

class OverlayState {
public:
    OverlayState();
    ~OverlayState();

    static OverlayState* get();
    
    OverlayInfo getOverlayInfo(const std::string& address);
    
    void registerWindow(Superglue* win);
    void unregisterWindow(Superglue* win);
    void log(const std::string& msg);
    void shutdown();

    // Setup the event loop hook
    void init();
    int onEvent(int fd, uint32_t mask);

private:
    void pollingLoop();
    void dispatchDamage(); // Writes to pipe

    std::unordered_set<std::string> m_mutedAddresses;
    // Map window address to pair<Type, Timestamp>
    std::unordered_map<std::string, std::pair<OverlayType, std::chrono::steady_clock::time_point>> m_volumeEvents;
    
    std::unordered_set<Superglue*> m_windows;
    const std::string MUTE_STATE_FILE = "/tmp/volume-mute-state";
    const std::string OVERLAY_CMD_FILE = "/tmp/superglue-overlay-cmd";
    
    std::thread m_pollThread;
    std::mutex m_mutex;
    bool m_running = true;

    // Pipe for signaling main thread
    int m_eventFd[2];
    wl_event_source* m_eventSource = nullptr;
};

extern std::unique_ptr<OverlayState> g_pOverlayState;
