cmake_minimum_required(VERSION 3.21)
project(rhi-widget LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui GuiPrivate ShaderTools)
set(CMAKE_AUTOMOC ON)

qt_add_executable(rhi-widget
    main.cpp
    cuberhiwidget.cpp
    cuberhiwidget.h
    model.cpp
    model.h

)
qt_add_resources(rhi-widget "rhi-widget"
    PREFIX
        "/"
    FILES
        "shader_assets/cube.vert.qsb"
        "shader_assets/cube.frag.qsb"
        "shaders/cube.frag"
        "shaders/cube.vert"
        "text.jpg"
)
target_link_libraries(rhi-widget
    PRIVATE Qt6::Widgets Qt6::Gui Qt6::GuiPrivate Qt6::ShaderTools
)
