#include "texture-cache.hpp"
#include <cairo/cairo.h>

TextureCache& TextureCache::get() {
  static TextureCache instance;
  return instance;
}

SP<CTexture> TextureCache::load(const std::string& path) {
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_cache.find(path);
  if (it != m_cache.end()) {
    return it->second.texture;
  }

  return loadFromFile(path);
}

Vector2D TextureCache::getSize(const std::string& path) {
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_cache.find(path);
  if (it != m_cache.end()) {
    return it->second.size;
  }
  return {0, 0};
}

void TextureCache::clear() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_cache.clear();
}

SP<CTexture> TextureCache::loadFromFile(const std::string& path) {
  cairo_surface_t* surface =
      cairo_image_surface_create_from_png(path.c_str());

  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(surface);
    return nullptr;
  }

  int w = cairo_image_surface_get_width(surface);
  int h = cairo_image_surface_get_height(surface);
  const auto data = cairo_image_surface_get_data(surface);

  SP<CTexture> tex = makeShared<CTexture>();
  tex->allocate();
  glBindTexture(GL_TEXTURE_2D, tex->m_texID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifndef GLES2
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

  glTexImage2D(
      GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, data);

  cairo_surface_destroy(surface);

  CachedTexture cached;
  cached.texture = tex;
  cached.size = {(double)w, (double)h};
  m_cache[path] = cached;

  return tex;
}
