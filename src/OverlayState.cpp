#include "OverlayState.hpp"
#include "Superglue.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sstream>
#include <mutex>
#include <hyprland/src/Compositor.hpp>

// Global instance
std::unique_ptr<OverlayState> g_pOverlayState;

OverlayState* OverlayState::get() {
    return g_pOverlayState.get();
}

// Static trampoline for the C callback
static int handleEvent(int fd, uint32_t mask, void* data) {
    return ((OverlayState*)data)->onEvent(fd, mask);
}

OverlayState::OverlayState() {
    log("OverlayState Constructor - starting thread");
    if (pipe(m_eventFd) != 0) {
        log("Failed to create pipe!");
    }
    m_pollThread = std::thread(&OverlayState::pollingLoop, this);
}

OverlayState::~OverlayState() {
    log("OverlayState Destructor");
    shutdown();
    if (m_eventSource) wl_event_source_remove(m_eventSource);
    close(m_eventFd[0]);
    close(m_eventFd[1]);
}

void OverlayState::init() {
    // This must be called from the main thread (e.g. plugin init)
    if (g_pCompositor && g_pCompositor->m_wlDisplay) {
        wl_event_loop* loop = wl_display_get_event_loop(g_pCompositor->m_wlDisplay);
        m_eventSource = wl_event_loop_add_fd(loop, m_eventFd[0], WL_EVENT_READABLE, handleEvent, this);
        log("Event loop hook registered.");
    } else {
        log("FATAL: Compositor or Display not available during init!");
    }
}

int OverlayState::onEvent(int fd, uint32_t mask) {
    if (mask & WL_EVENT_READABLE) {
        // log("Main thread received event!");
        char buf[8];
        read(fd, buf, sizeof(buf)); // Drain the pipe
        
        // Now we are on the main thread, safe to damage!
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_windows.empty()) log("Warning: No windows registered to damage!");
        for (auto* win : m_windows) { 
            if (win) win->damageEntire(); 
        }
    }
    return 0;
}

void OverlayState::dispatchDamage() {
    // log("Dispatching damage to main thread..."); 
    char c = 1;
    if (write(m_eventFd[1], &c, 1) < 0) {
        log("Failed to write to pipe!");
    }
}

void OverlayState::shutdown() {
    m_running = false;
    if (m_pollThread.joinable()) m_pollThread.join();
}

void OverlayState::log(const std::string& msg) {
    // Simple file lock or just open/close for now, but let's make it atomic-ish
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream f("/tmp/superglue.log", std::ios::app);
    f << msg << std::endl;
    f.flush();
}

void OverlayState::pollingLoop() {
    log("Polling loop STARTED. m_running: " + std::to_string(m_running));
    
    // Initial read
    {
         std::ifstream f(MUTE_STATE_FILE);
         if (f.is_open()) {
             std::string line;
             while (std::getline(f, line)) {
                 if (!line.empty()) m_mutedAddresses.insert(line);
             }
         }
    }

    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Faster poll for volume events
        
        // static int heartbeat = 0;
        // if (heartbeat++ % 100 == 0) log("Polling loop heartbeat...");

        // 1. Read Mute State
        std::unordered_set<std::string> newAddresses;
        std::ifstream f(MUTE_STATE_FILE);
        if (f.is_open()) {
            std::string line;
            while (std::getline(f, line)) {
                if (!line.empty()) newAddresses.insert(line);
            }
        }
        
        bool changed = false;
        if (newAddresses.size() != m_mutedAddresses.size()) changed = true;
        else {
             for (const auto& addr : newAddresses) {
                 if (!m_mutedAddresses.contains(addr)) {
                     changed = true; 
                     break;
                 }
             }
        }

        if (changed) {
            log("State changed! Triggering updates.");
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_mutedAddresses = newAddresses;
            }
            dispatchDamage();
        }

        // 2. Read Overlay Commands (Volume Up/Down)
        std::ifstream cmdFile(OVERLAY_CMD_FILE);
        if (cmdFile.is_open()) {
            std::string line;
            bool damageNeeded = false;
            while (std::getline(cmdFile, line)) {
                if (line.empty()) continue;
                std::stringstream ss(line);
                std::string cmd;
                std::string addr;
                ss >> cmd >> addr;
                
                if (!addr.empty()) {
                    log("Received cmd: " + cmd + " for " + addr);
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (cmd == "vol-up") {
                        m_volumeEvents[addr] = {OverlayType::VolumeUp, std::chrono::steady_clock::now()};
                        damageNeeded = true;
                    } else if (cmd == "vol-down") {
                        m_volumeEvents[addr] = {OverlayType::VolumeDown, std::chrono::steady_clock::now()};
                        damageNeeded = true;
                    }
                }
            }
            // Clear the cmd file
            cmdFile.close();
            std::ofstream clearCmd(OVERLAY_CMD_FILE, std::ios::trunc); 
        
            if (damageNeeded) {
               log("Damage needed for volume event");
               dispatchDamage();
            }
        }
        
        // 3. Check for expired volume events to damage them out
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto now = std::chrono::steady_clock::now();
            bool cleanupNeeded = false;
            for (auto it = m_volumeEvents.begin(); it != m_volumeEvents.end();) {
                 auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.second).count();
                 if (elapsed > 1000) { // Total animation time
                     it = m_volumeEvents.erase(it);
                     cleanupNeeded = true; 
                 } else {
                     // Animation still active
                     cleanupNeeded = true;
                     ++it;
                 }
            }
            if (cleanupNeeded) dispatchDamage();
        }
    }
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

OverlayInfo OverlayState::getOverlayInfo(const std::string& address) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Check Volume Events (Transient)
    if (m_volumeEvents.contains(address)) {
        auto& event = m_volumeEvents[address];
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - event.second).count();
        
        if (elapsed < 800) {
            // Full opacity for 800ms
            return {event.first, 1.0f};
        } else if (elapsed < 1000) {
            // Fade out 200ms
            float alpha = 1.0f - (float)(elapsed - 800) / 200.0f;
            return {event.first, alpha};
        } else {
             // Expired, will be cleaned up by loop
        }
    }

    // 2. Check Persistent Mute
    if (m_mutedAddresses.contains(address)) {
        return {OverlayType::Mute, 1.0f};
    }

    return {OverlayType::None, 0.0f};
}
