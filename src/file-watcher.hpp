#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

/**
 * Watches files for changes and triggers callbacks.
 * Runs in a background thread with configurable poll interval.
 */
class FileWatcher {
 public:
  using Callback = std::function<void(const std::string&)>;

  FileWatcher(int pollIntervalMs = 10);
  ~FileWatcher();

  /**
   * Starts watching a file and calls callback when content changes.
   */
  void watch(const std::string& path, Callback callback);

  /**
   * Stops the watcher thread.
   */
  void stop();

 private:
  void runLoop();
  std::string readFile(const std::string& path);

  struct WatchEntry {
    std::string path;
    std::string lastContent;
    Callback callback;
  };

  std::vector<WatchEntry> m_watches;
  std::thread m_thread;
  std::atomic<bool> m_running{true};
  int m_pollIntervalMs;
  std::mutex m_mutex;
};
