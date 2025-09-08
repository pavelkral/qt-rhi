cmake_minimum_required(VERSION 3.16)
project(single-cube LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate)

qt_standard_project_setup()

qt_add_executable(single-cube
    main.cpp
    rhiwindow.cpp rhiwindow.h
    cube.h
    cube.h cube.cpp
    camera.h camera.cpp
)

set_target_properties(single-cube PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(single-cube PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
)

set_source_files_properties("shaders/prebuild/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set_source_files_properties("shaders/prebuild/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("shaders/prebuild/quad.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "quad.vert.qsb"
)
set_source_files_properties("shaders/prebuild/quad.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "quad.frag.qsb"
)
set_source_files_properties("shaders/prebuild/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set_source_files_properties("shaders/prebuild/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("shaders/prebuild/mcolor.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "mcolor.vert.qsb"
)
set_source_files_properties("shaders/prebuild/mcolor.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "mcolor.frag.qsb"
)
qt_add_resources(single-cube "single-cube"
    PREFIX
        "/"
    FILES
        "shaders/prebuild/color.vert.qsb"
        "shaders/prebuild/color.frag.qsb"
        "shaders/prebuild/mcolor.vert.qsb"
        "shaders/prebuild/mcolor.frag.qsb"
        "shaders/prebuild/quad.vert.qsb"
        "shaders/prebuild/quad.frag.qsb"
        "shaders/prebuild/texture.vert.qsb"
        "shaders/prebuild/texture.frag.qsb"
        "shaders/color.vert"
        "shaders/color.frag"
        "shaders/mcolor.vert"
        "shaders/mcolor.frag"
        "shaders/quad.vert"
        "shaders/quad.frag"
        "shaders/texture.frag"
        "shaders/texture.vert"
        "text.jpg"
)

install(TARGETS single-cube
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET single-cube
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
