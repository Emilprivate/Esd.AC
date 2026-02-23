cmake_minimum_required(VERSION 3.15)
project(AssaultCubeHack VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Resolve MinHook (local first, fallback to fetch)
set(MINHOOK_DIR "${CMAKE_SOURCE_DIR}/ext/minhook")
if (EXISTS "${MINHOOK_DIR}/CMakeLists.txt")
    add_subdirectory("${MINHOOK_DIR}")
else()
    message(STATUS "MinHook not found in ext/minhook, fetching from GitHub...")
    FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(minhook)
endif()

# Resolve Dear ImGui (local first, fallback to fetch)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/ext/imgui")
if (NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(STATUS "Dear ImGui not found in ext/imgui, fetching from GitHub...")
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG master
    )
    FetchContent_GetProperties(imgui)
    if (NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()
    set(IMGUI_DIR "${imgui_SOURCE_DIR}")
endif()

# Add ImGui
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp
)

# Create the shared library (DLL)
add_library(${PROJECT_NAME} SHARED
    src/core/main.cpp
    src/core/hooking/hook.cpp
    src/ui/gui.cpp
    src/ui/gui_theme.cpp
    src/ui/gui_menu.cpp
    src/features/hack_logic.cpp
    src/features/esp/esp_feature.cpp
    src/features/aimbot/aimbot_feature.cpp
    src/config/config.cpp
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ext/minhook/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    minhook
    opengl32
)

# Set target properties for Windows DLL
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    OUTPUT_NAME "esd.ac"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config.json
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json
)
