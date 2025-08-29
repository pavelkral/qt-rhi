cmake_minimum_required(VERSION 3.21)
project(qt-rhi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)


add_subdirectory(rhiwindow)
add_subdirectory(simplerhiwidget)
add_subdirectory(cuberhiwidget)
add_subdirectory(rhi-cube-window)
add_subdirectory(rhi-cube-widget)
add_subdirectory(rhi-box)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT rhiwindow)
