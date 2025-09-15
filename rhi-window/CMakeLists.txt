cmake_minimum_required(VERSION 3.16)
project(rhi-window LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate)

qt_standard_project_setup()

qt_add_executable(rhi-window
    main.cpp
    rhiwindow.cpp rhiwindow.h
    model.h
    model.h model.cpp
    camera.h camera.cpp
    geometry.h
    transform.h transform.cpp

)

set_target_properties(rhi-window PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(rhi-window PRIVATE
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
set_source_files_properties("shaders/prebuild/light.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "light.vert.qsb"
)
set_source_files_properties("shaders/prebuild/light.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "light.frag.qsb"
)
set_source_files_properties("shaders/prebuild/pbr.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "pbr.vert.qsb"
)
set_source_files_properties("shaders/prebuild/pbr.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "pbr.frag.qsb"
)
qt_add_resources(rhi-window "rhi-window"
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
        "shaders/prebuild/light.vert.qsb"
        "shaders/prebuild/light.frag.qsb"
        "shaders/prebuild/pbr.vert.qsb"
        "shaders/prebuild/pbr.frag.qsb"
        "shaders/color.vert.hlsl"
        "shaders/color.frag.hlsl"
        "shaders/mcolor.vert"
        "shaders/mcolor.frag"
        "shaders/quad.vert"
        "shaders/quad.frag"
        "shaders/texture.frag"
        "shaders/texture.vert"
        "shaders/light.frag"
        "shaders/light.vert"
        "shaders/pbr.frag"
        "shaders/pbr.vert"
        "../assets/textures/floor.jpg"
        "../assets/textures/floor.png"
         "../assets/textures/brick/victorian-brick_albedo.png"
         "../assets/textures/brick/victorian-brick_ao.png"
         "../assets/textures/brick/victorian-brick_height.png"
         "../assets/textures/brick/victorian-brick_metallic.png"
         "../assets/textures/brick/victorian-brick_normal-ogl.png"
         "../assets/textures/brick/victorian-brick_roughness.png"
)

install(TARGETS rhi-window
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET rhi-window
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
