# --- Začátek bloku pro FetchContent ---
include(FetchContent)

# Tímhle vypneme buildování testů a nástrojů, které assimp má,
# urychlí to první konfiguraci projektu.
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable assimp tests")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable assimp tools")
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "Disable assimp samples")
set(ASSIMP_BUILD_SHARED_LIBS OFF CACHE BOOL "Build assimp as static lib")

# Deklarace, odkud se má assimp stáhnout
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    # Použij stabilní tag, např. v5.4.1. Můžeš změnit podle potřeby.
    GIT_TAG        v5.4.1
)
# Tohle provede stažení, konfiguraci a build (pokud je to nutné)
FetchContent_MakeAvailable(assimp)

# --- Konec bloku pro FetchContent ---
# Původní kód (už bez find_package a if bloku)
file(GLOB_RECURSE SOURCES *.cpp *.h)
add_executable(tutorial-11 ${SOURCES})

target_compile_options(tutorial-11
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /Zc:preprocessor /Zc:__cplusplus /wd5054 /bigobj>
        $<$<CXX_COMPILER_ID:MSVC>:/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR> # VS 17.10: https://github.com/microsoft/STL/issues/4730
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wno-deprecated-enum-enum-conversion>
)

target_link_libraries(tutorial-11
    PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::GuiPrivate
        assimp::assimp  # Tento target nám teď vytvořil FetchContent
)

qt_add_shaders(tutorial-11 "shaders"
    PREFIX
        "/"
    FILES
        "vertex.vert"
        "fragment.frag"
)
