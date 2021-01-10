cmake_minimum_required(VERSION 3.17)
project(ForkerRenderer)

set(CMAKE_CXX_STANDARD 14)

find_package(spdlog REQUIRED)

add_executable(ForkerRenderer
    src/tgaimage.cpp
    src/geometry.cpp
    src/model.cpp
    src/mesh.cpp
    src/buffer.cpp
    src/forkergl.cpp
    src/camera.cpp
    src/main.cpp)

target_link_libraries(ForkerRenderer PRIVATE spdlog::spdlog)