#pragma once

#include <hyprland/src/render/OpenGL.hpp>
#include <string>
#include <unordered_map>
#include <mutex>

/**
 * Caches OpenGL textures loaded from PNG files.
 * Thread-safe singleton for texture management.
 */
class TextureCache {
 public:
  static TextureCache& get();

  /**
   * Loads a texture from path, returning cached version if available.
   * Returns nullptr on failure.
   */
  SP<CTexture> load(const std::string& path);

  /**
   * Gets the size of a loaded texture.
   */
  Vector2D getSize(const std::string& path);

  /**
   * Clears all cached textures.
   */
  void clear();

 private:
  TextureCache() = default;

  SP<CTexture> loadFromFile(const std::string& path);

  struct CachedTexture {
    SP<CTexture> texture;
    Vector2D size;
  };

  std::unordered_map<std::string, CachedTexture> m_cache;
  std::mutex m_mutex;
};
