# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(cube-window LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate)

qt_standard_project_setup()

qt_add_executable(cube-window
    main.cpp
    rhiwindow.cpp rhiwindow.h
    cube.h
)

set_target_properties(cube-window PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(cube-window PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
)

set_source_files_properties("shaders/prebuilt/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set_source_files_properties("shaders/prebuilt/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("shaders/prebuilt/quad.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "quad.vert.qsb"
)
set_source_files_properties("shaders/prebuilt/quad.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "quad.frag.qsb"
)
set_source_files_properties("shaders/prebuilt/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set_source_files_properties("shaders/prebuilt/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)

qt_add_resources(cube-window "cube-window"
    PREFIX
        "/"
    FILES
        "shaders/prebuilt/color.vert.qsb"
        "shaders/prebuilt/color.frag.qsb"
        "shaders/prebuilt/quad.vert.qsb"
        "shaders/prebuilt/quad.frag.qsb"
        "shaders/prebuilt/texture.vert.qsb"
        "shaders/prebuilt/texture.frag.qsb"
        "shaders/color.vert"
        "shaders/color.frag"
        "shaders/quad.vert"
        "shaders/quad.frag"
        "shaders/texture.frag"
        "shaders/texture.vert"
        "text.jpg"
        "floor.png"
)

install(TARGETS cube-window
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET cube-window
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
