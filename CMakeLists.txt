cmake_minimum_required(VERSION 3.17)
project(ForkerRenderer)

set(CMAKE_CXX_STANDARD 14)

find_package(spdlog REQUIRED)

add_executable(ForkerRenderer
    tgaimage.cpp
    geometry.cpp
    model.cpp
    mesh.cpp
    buffer.cpp
    forkergl.cpp
    camera.cpp
    main.cpp)

target_link_libraries(ForkerRenderer PRIVATE spdlog::spdlog)