cmake_minimum_required(VERSION 3.16)
project(rhi-window LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate Gui Widgets)

qt_standard_project_setup()

qt_add_executable(rhi-window
    main.cpp
    rhiwindow.cpp
    rhiwindow.h
    qtrhi3d/model.h
    qtrhi3d/model.cpp
    qtrhi3d/camera.h
    qtrhi3d/camera.cpp
    qtrhi3d/geometry.h
    qtrhi3d/transform.h
    qtrhi3d/transform.cpp
    qtrhi3d/types.h

)

set_target_properties(rhi-window PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(rhi-window PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Widgets
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
        "shaders/prebuild/pbrvk.vert.qsb"
        "shaders/prebuild/pbrvk.frag.qsb"
        "shaders/prebuild/pbrd3d.vert.qsb"
        "shaders/prebuild/pbrd3d.frag.qsb"
        "shaders/prebuild/depth.vert.qsb"
        "shaders/prebuild/depth.frag.qsb"
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
        "shaders/pbrvk.frag"
        "shaders/pbrvk.vert"
        "shaders/pbrd3d.frag"
        "shaders/pbrd3d.vert"
        "shaders/depth.frag"
        "shaders/depth.vert"
        "../assets/textures/floor.jpg"
        "../assets/textures/floor.png"
         "../assets/textures/brick/victorian-brick_albedo.png"
         "../assets/textures/brick/victorian-brick_ao.png"
         "../assets/textures/brick/victorian-brick_height.png"
         "../assets/textures/brick/victorian-brick_metallic.png"
         "../assets/textures/brick/victorian-brick_normal-ogl.png"
         "../assets/textures/brick/victorian-brick_roughness.png"
         "../assets/textures/panel/sci-fi-panel1-albedo.png"
         "../assets/textures/panel/sci-fi-panel1-ao.png"
         "../assets/textures/panel/sci-fi-panel1-height.png"
         "../assets/textures/panel/sci-fi-panel1-metallic.png"
         "../assets/textures/panel/sci-fi-panel1-normal-ogl.png"
         "../assets/textures/panel/sci-fi-panel1-roughness.png"
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
