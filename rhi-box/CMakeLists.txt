# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rhibox LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate Widgets)

qt_standard_project_setup()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include

)

qt_add_executable(rhibox
    examplewidget.cpp examplewidget.h cube.h
    main.cpp
    RhiFBXModel.h

)

set_target_properties(rhibox PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)


# Platform-specific settings
if(WIN32)
    foreach(dll ${DLL_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dll} $<TARGET_FILE_DIR:${PROJECT_NAME}>

    )
    endforeach()
    # Windows-specific linking
    message(STATUS "Hodnota CMAKE_CURRENT_BINARY_DIR je: ${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(rhibox PRIVATE

        Qt6::Core
        Qt6::Gui
        Qt6::GuiPrivate
        Qt6::Widgets
        ${CMAKE_CURRENT_BINARY_DIR}/../../../libs/assimp/lib/assimp-vc143-mt.lib
    )

    # DLLs to copy
    set(DLL_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../libs/assimp/assimp-vc143-mt.dll

    )

else() # Unix/Linux/Mac
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(assimp REQUIRED)

    target_link_libraries(${PROJECT_NAME}

        ${ASSIMP_LIBRARIES}
        # IrrKlang is Windows only; remove or handle separately for Unix
    )
endif()
# needs GuiPrivate to be able to include <rhi/qrhi.h>


qt_add_resources(rhibox "rhibox"
    PREFIX
        "/"
    FILES
        "shaders/prebuild/model.vert.qsb"
        "shaders/prebuild/model.frag.qsb"
        "shaders/model.vert"
        "shaders/model.frag"
        "shaders/prebuild/cube.vert.qsb"
        "shaders/prebuild/lambert.frag.qsb"
        "shaders/prebuild/toon.frag.qsb"
        "shaders/cube.vert"
        "shaders/lambert.frag"
        "shaders/toon.frag"
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
