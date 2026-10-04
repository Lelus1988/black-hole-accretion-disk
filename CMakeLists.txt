cmake_minimum_required(VERSION 3.16)
project(BlackHole VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# A raylib binary from the system may only create an OpenGL 3.3 context.
# Build raylib with its 4.3 backend by default so compute shaders are usable.
option(BLACKHOLE_USE_SYSTEM_RAYLIB "Use an installed raylib 6.0+ built for OpenGL 4.3" OFF)

if(BLACKHOLE_USE_SYSTEM_RAYLIB)
    find_package(raylib 6.0 REQUIRED)
else()
    # Fetch the rendering dependencies so a clean clone can build without local installs.
    include(FetchContent)
    set(OPENGL_VERSION "4.3" CACHE STRING "OpenGL version used by raylib" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Disable raylib examples" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 6.0
    )
    FetchContent_MakeAvailable(raylib)
endif()

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG Raylib_6_0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rlimgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

add_library(rlimgui STATIC ${rlimgui_SOURCE_DIR}/rlImGui.cpp)
target_include_directories(rlimgui PUBLIC ${rlimgui_SOURCE_DIR})
target_compile_definitions(rlimgui PUBLIC NO_FONT_AWESOME)
target_link_libraries(rlimgui PUBLIC imgui raylib)

# Source files
set(SOURCES
    src/main.cpp
    src/App.cpp
    src/Renderer.cpp
    src/Camera.cpp
    src/Scene.cpp
    src/ParticleSystem.cpp
    src/GpuParticleSystem.cpp
)

set(HEADERS
    src/App.h
    src/Renderer.h
    src/Camera.h
    src/Scene.h
    src/Particle.h
    src/ParticleSystem.h
    src/GpuParticleSystem.h
    src/Gravity.h
    src/Settings.h
    src/PerformanceMonitor.h
)

# Add raylib definitions for OpenGL 4.3 (needed for compute shaders)
add_definitions(-DGRAPHICS_API_OPENGL_43 -DPLATFORM_DESKTOP)

# Utility headers
set(UTIL_HEADERS
    src/utils/Math.h
    src/utils/Random.h
    src/utils/Timer.h
    src/utils/Logger.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UTIL_HEADERS})

# Link raylib
if(TARGET raylib)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib::raylib)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE rlimgui imgui)

# Compiler warnings and optimizations
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /O2)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -O3 -march=native)
endif()

# The executable reads shaders at runtime, so copy them beside each fresh build.
file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/shaders/*")
set(SHADER_STAMP "${CMAKE_BINARY_DIR}/bin/shaders/.shader-copy-stamp")
add_custom_command(
    OUTPUT "${SHADER_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders"
            "${CMAKE_BINARY_DIR}/bin/shaders"
    COMMAND ${CMAKE_COMMAND} -E touch "${SHADER_STAMP}"
    DEPENDS ${SHADER_FILES}
    COMMENT "Copying runtime shaders"
)
add_custom_target(BlackHoleShaders DEPENDS "${SHADER_STAMP}")
add_dependencies(${PROJECT_NAME} BlackHoleShaders)
