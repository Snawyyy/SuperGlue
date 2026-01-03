cmake_minimum_required(VERSION 3.19)
project(superglue)

set(CMAKE_CXX_STANDARD 23)

# Include paths
include_directories(/usr/include/hyprland)
include_directories(/usr/include/pixman-1)
include_directories(/usr/include/libdrm)
include_directories(/usr/include/hyprland/protocols)
include_directories(/usr/include/cairo)
include_directories(src)

# Source files - modular architecture
set(SOURCES
  src/main.cpp
  src/decoration.cpp
  src/overlay-state.cpp
  src/file-watcher.cpp
  src/texture-cache.cpp
  src/pass-element.cpp
)

add_library(superglue SHARED ${SOURCES})

target_link_libraries(superglue
  stdc++
  pixman-1
  drm
  GLESv2
  cairo
  wayland-server
  pthread
)

# Plugin naming
set_target_properties(superglue PROPERTIES PREFIX "")
