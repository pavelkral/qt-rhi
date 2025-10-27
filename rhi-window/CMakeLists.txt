cmake_minimum_required(VERSION 3.16)
project(rhi-window LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate Gui Widgets)
add_subdirectory(utils)
qt_standard_project_setup()
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include

)

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
    qtrhi3d/proceduralsky.h
    qtrhi3d/hdrisky.h
    qtrhi3d/hdrisky.cpp
    qtrhi3d/proceduralsky.cpp
    qtrhi3d/apifuturesinfo.h
    qtrhi3d/fbxmodel.h qtrhi3d/fbxmodel.cpp
    qtrhi3d/assimputils.h
)

target_include_directories(rhi-window PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/utils

)

set_target_properties(rhi-window PROPERTIES
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
    target_link_libraries(rhi-window PRIVATE

        Qt6::Core
        Qt6::Gui
        Qt6::GuiPrivate
        Qt6::Widgets
		utils
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
        "shaders/prebuild/pcgsky.vert.qsb"
        "shaders/prebuild/pcgsky.frag.qsb"
        "shaders/prebuild/mainpbr.frag.qsb"
        "shaders/prebuild/model.vert.qsb"
        "shaders/prebuild/model.frag.qsb"
        "shaders/prebuild/skybox.vert.qsb"
        "shaders/prebuild/skybox.frag.qsb"
        "shaders/prebuild/equirect2cube.vert.qsb"
        "shaders/prebuild/equirect2cube.frag.qsb"

        "shaders/skybox.vert"
        "shaders/skybox.frag"
        "shaders/equirect2cube.vert"
        "shaders/equirect2cube.frag"
        #"shaders/color.vert.hlsl"
        #"shaders/color.frag.hlsl"
        "shaders/mcolor.vert"
        "shaders/mcolor.frag"
        "shaders/model.vert"
        "shaders/model.frag"
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
        "shaders/mainpbr.frag"
        "shaders/depth.frag"
        "shaders/depth.vert"
        "shaders/pcgsky.frag"
        "shaders/pcgsky.vert"
        "../assets/textures/floor.png"
        "../assets/textures/floorN.png"
        "../assets/textures/floorM.png"
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
         "../assets/textures/sky.hdr"
         "../assets/textures/sky.png"
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
