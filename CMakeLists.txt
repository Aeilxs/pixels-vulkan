cmake_minimum_required(VERSION 3.28)

project(
    Pixel_Storm
    VERSION 0.1.0
    LANGUAGES CXX
)

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

set(TRIANGLE_VERTEX_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert")
set(TRIANGLE_FRAGMENT_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag")
set(TRIANGLE_VERTEX_SPIRV "${PIXEL_STORM_SHADER_OUTPUT_DIR}/triangle.vert.spv")
set(TRIANGLE_FRAGMENT_SPIRV "${PIXEL_STORM_SHADER_OUTPUT_DIR}/triangle.frag.spv")

pixel_storm_compile_shader("${TRIANGLE_VERTEX_SHADER}" "${TRIANGLE_VERTEX_SPIRV}")
pixel_storm_compile_shader("${TRIANGLE_FRAGMENT_SHADER}" "${TRIANGLE_FRAGMENT_SPIRV}")

add_custom_target(
    Pixel_StormShaders
    DEPENDS
        "${TRIANGLE_VERTEX_SPIRV}"
        "${TRIANGLE_FRAGMENT_SPIRV}"
)

#
# Executable
#

add_executable(
    Pixel_Storm
    src/main.cpp

    src/app/app.cpp
    src/app/cli.cpp

    src/gfx/camera/camera_2d.cpp
    src/gfx/particles/system.cpp

    src/image/image.cpp

    src/platform/sdl_context.cpp
    src/platform/window.cpp

    src/renderer/vulkan/buffer.cpp
    src/renderer/vulkan/command_buffer.cpp
    src/renderer/vulkan/command_pool.cpp
    src/renderer/vulkan/device.cpp
    src/renderer/vulkan/frame_synchronization.cpp
    src/renderer/vulkan/graphics_pipeline.cpp
    src/renderer/vulkan/instance.cpp
    src/renderer/vulkan/physical_device.cpp
    src/renderer/vulkan/renderer.cpp
    src/renderer/vulkan/surface.cpp
    src/renderer/vulkan/swapchain.cpp
)

add_dependencies(Pixel_Storm Pixel_StormShaders)

target_include_directories(
    Pixel_Storm
    SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(
    Pixel_Storm
    PRIVATE
        PIXEL_STORM_SHADER_DIR="${PIXEL_STORM_SHADER_OUTPUT_DIR}"
)

target_compile_features(
    Pixel_Storm
    PRIVATE cxx_std_23
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
