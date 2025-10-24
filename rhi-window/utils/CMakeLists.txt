project(utils LANGUAGES CXX)

add_library(utils SHARED
    src/jsonutils.cpp
    src/jsonutils.h
    src/logger.cpp
    src/logger.h
    src/colorpicker.cpp
    src/colorpicker.h
    src/utils.h
    src/stringutils.h
    src/stringutils.cpp
)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Xml Network Widgets)

target_compile_definitions(utils PRIVATE UTILS)

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(utils
    PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
    Qt6::Network
    Qt6::Widgets
)
include(GNUInstallDirs)

install(TARGETS utils
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET utils
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
