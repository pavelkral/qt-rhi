# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rhibox LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate Widgets)

qt_standard_project_setup()

qt_add_executable(rhibox
    examplewidget.cpp examplewidget.h cube.h
    main.cpp
)

set_target_properties(rhibox PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# needs GuiPrivate to be able to include <rhi/qrhi.h>
target_link_libraries(rhibox PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Widgets
)

qt_add_resources(rhibox "rhibox"
    PREFIX
        "/"
    FILES
        "shader_assets/texture.vert.qsb"
        "shader_assets/texture.frag.qsb"
        "shaders/texture.vert"
        "shaders/texture.frag"
)

install(TARGETS rhibox
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET rhibox
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
