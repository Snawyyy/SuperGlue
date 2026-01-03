#include "file-watcher.hpp"
#include <fstream>
#include <sstream>

FileWatcher::FileWatcher(int pollIntervalMs)
    : m_pollIntervalMs(pollIntervalMs) {
  m_thread = std::thread(&FileWatcher::runLoop, this);
}

FileWatcher::~FileWatcher() {
  stop();
}

void FileWatcher::watch(
    const std::string& path,
    Callback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  WatchEntry entry;
  entry.path = path;
  entry.lastContent = readFile(path);
  entry.callback = callback;
  m_watches.push_back(entry);
}

void FileWatcher::stop() {
  m_running = false;
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

std::string FileWatcher::readFile(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) return "";
  std::stringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

void FileWatcher::runLoop() {
  while (m_running) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(m_pollIntervalMs));

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& entry : m_watches) {
      std::string content = readFile(entry.path);
      if (content != entry.lastContent) {
        entry.lastContent = content;
        entry.callback(content);
      }
    }
  }
}
