cmake_minimum_required(VERSION 3.28)

project(
    Pixel_Storm
    VERSION 0.1.0
    LANGUAGES C CXX
)

#
# Dependencies
#
# - Vulkan SDK: https://vulkan.lunarg.com/sdk/home
# - SDL3: https://www.libsdl.org/
# - GLM: https://github.com/g-truc/glm
# - stb: https://github.com/nothings/stb (stb_image.h, stb_truetype.h)
# - Log: Rxi's log library: https://github.com/rxi/log.c

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#
# Packages
#

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)

#
# Shaders
#

set(PIXEL_STORM_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

function(pixel_storm_compile_shader shader_source shader_output)
    add_custom_command(
        OUTPUT "${shader_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${PIXEL_STORM_SHADER_OUTPUT_DIR}"
        COMMAND "${GLSLANG_VALIDATOR}"
                -V
                --target-env vulkan1.3
                "${shader_source}"
                -o "${shader_output}"
        DEPENDS "${shader_source}"
        VERBATIM
    )
endfunction()

set(PARTICLE_VERTEX_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.vert")
set(PARTICLE_FRAGMENT_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.frag")
set(TEXT_VERTEX_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/shaders/text.vert")
set(TEXT_FRAGMENT_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/shaders/text.frag")

set(PARTICLE_VERTEX_SPIRV "${PIXEL_STORM_SHADER_OUTPUT_DIR}/particle.vert.spv")
set(PARTICLE_FRAGMENT_SPIRV "${PIXEL_STORM_SHADER_OUTPUT_DIR}/particle.frag.spv")
set(TEXT_VERTEX_SPIRV "${PIXEL_STORM_SHADER_OUTPUT_DIR}/text.vert.spv")
set(TEXT_FRAGMENT_SPIRV "${PIXEL_STORM_SHADER_OUTPUT_DIR}/text.frag.spv")

pixel_storm_compile_shader("${PARTICLE_VERTEX_SHADER}" "${PARTICLE_VERTEX_SPIRV}")
pixel_storm_compile_shader("${PARTICLE_FRAGMENT_SHADER}" "${PARTICLE_FRAGMENT_SPIRV}")
pixel_storm_compile_shader("${TEXT_VERTEX_SHADER}" "${TEXT_VERTEX_SPIRV}")
pixel_storm_compile_shader("${TEXT_FRAGMENT_SHADER}" "${TEXT_FRAGMENT_SPIRV}")

add_custom_target(
    Pixel_StormShaders
    DEPENDS
        "${PARTICLE_VERTEX_SPIRV}"
        "${PARTICLE_FRAGMENT_SPIRV}"
        "${TEXT_VERTEX_SPIRV}"
        "${TEXT_FRAGMENT_SPIRV}"
)

#
# Executable
#

add_executable(
    Pixel_Storm
    src/main.cpp

    src/app/app.cpp
    src/app/benchmark.cpp
    src/app/cli.cpp
    src/app/diagnostics.cpp
    src/app/frame_stats.cpp

    src/gfx/camera/camera_2d.cpp
    src/gfx/fonts/font_atlas.cpp
    src/gfx/fonts/text_geometry.cpp
    src/gfx/particles/system.cpp

    src/image/image.cpp

    src/platform/sdl_context.cpp
    src/platform/window.cpp

    src/vulkan/buffer.cpp
    src/vulkan/command_buffer.cpp
    src/vulkan/command_pool.cpp
    src/vulkan/debug_messenger.cpp
    src/vulkan/device.cpp
    src/vulkan/frame_synchronization.cpp
    src/vulkan/graphics_pipeline.cpp
    src/vulkan/image.cpp
    src/vulkan/instance.cpp
    src/vulkan/physical_device.cpp
    src/vulkan/renderer.cpp
    src/vulkan/surface.cpp
    src/vulkan/swapchain.cpp
    src/vulkan/text_overlay.cpp

    vendor/log/log.c
)

add_dependencies(Pixel_Storm Pixel_StormShaders)

target_include_directories(
    Pixel_Storm
    SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/log"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_options(
    Pixel_Storm
    PRIVATE
        # Keep __FILE__ paths relative to the project root.
        "-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/="
        -Wall
        -Wextra
        -Werror
)

# Debug
#   logs        DEBUG
#   validation ON
#   assertions ON
# 
# RelWithDebInfo
#   logs        INFO
#   validation OFF
#   optimisation + symboles
#   assertions OFF
# 
# Release
#   logs        WARN
#   validation OFF
#   optimisation
#   assertions OFF

target_compile_definitions(
    Pixel_Storm
    PRIVATE
        PIXEL_STORM_SHADER_DIR="${PIXEL_STORM_SHADER_OUTPUT_DIR}"
        PIXEL_STORM_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
        LOG_USE_COLOR

        $<$<CONFIG:Debug>:APPLICATION_LOG_LEVEL=LOG_TRACE>
        $<$<CONFIG:Debug>:ENABLE_VULKAN_VALIDATION>

        $<$<CONFIG:Release>:APPLICATION_LOG_LEVEL=LOG_TRACE>
        $<$<CONFIG:RelWithDebInfo>:APPLICATION_LOG_LEVEL=LOG_TRACE>
        $<$<CONFIG:MinSizeRel>:APPLICATION_LOG_LEVEL=LOG_TRACE>
)

target_compile_features(
    Pixel_Storm
    PRIVATE cxx_std_23 c_std_99
)

target_link_libraries(
    Pixel_Storm
    PRIVATE
        SDL3::SDL3
        Vulkan::Vulkan
        glm::glm
)

set_target_properties(
    Pixel_Storm
    PROPERTIES CXX_EXTENSIONS OFF
)

include(cmake/build_info.cmake)
