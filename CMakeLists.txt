cmake_minimum_required(VERSION 3.17)
project(ForkerRenderer)

set(CMAKE_CXX_STANDARD 14)

find_package(spdlog REQUIRED)

add_executable(ForkerRenderer
    # Core
    src/tgaimage.cpp
    src/geometry.cpp
    src/model.cpp
    src/mesh.cpp
    src/buffer.cpp
    src/forkergl.cpp
    src/camera.cpp
    # Shaders
    src/shaders/shadow.cpp
    # Main
    src/main.cpp)

target_include_directories(ForkerRenderer PRIVATE src src/shaders)
target_link_libraries(ForkerRenderer PRIVATE spdlog::spdlog)

# Create output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/output)