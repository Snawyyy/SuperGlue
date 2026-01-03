cmake_minimum_required(VERSION 3.19)
project(superglue)

set(CMAKE_CXX_STANDARD 23)

# Add Hyprland headers path here if not in standard include
include_directories(/usr/include/hyprland)
include_directories(/usr/include/pixman-1)
include_directories(/usr/include/libdrm)
include_directories(/usr/include/hyprland/protocols)
include_directories(/usr/include/cairo)
include_directories(src)

# We must include MuteState.cpp because Superglue uses it
add_library(superglue SHARED src/main.cpp src/Superglue.cpp src/OverlayState.cpp src/GluePassElement.cpp)

target_link_libraries(superglue stdc++ pixman-1 drm GLESv2 cairo wayland-server pthread)

# Important for plugins
set_target_properties(superglue PROPERTIES PREFIX "")
