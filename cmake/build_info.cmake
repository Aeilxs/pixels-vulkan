find_package(Git QUIET)

set(PIXEL_STORM_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(PIXEL_STORM_BUILD_INFO_HEADER "${PIXEL_STORM_GENERATED_DIR}/build/build_info.hpp")
set(PIXEL_STORM_BUILD_INFO_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_info.hpp.in")

add_custom_target(
    Pixel_StormBuildInfo ALL
    COMMAND
        "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DTEMPLATE_FILE=${PIXEL_STORM_BUILD_INFO_TEMPLATE}"
        "-DOUTPUT_FILE=${PIXEL_STORM_BUILD_INFO_HEADER}"
        "-DGIT_EXECUTABLE=${GIT_EXECUTABLE}"
        "-DBUILD_PROJECT_NAME=${PROJECT_NAME}"
        "-DBUILD_PROJECT_VERSION=${PROJECT_VERSION}"
        "-DBUILD_CONFIGURATION=$<CONFIG>"
        "-DBUILD_COMPILER=${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_build_info.cmake"
    BYPRODUCTS
        "${PIXEL_STORM_BUILD_INFO_HEADER}"
    COMMENT
        "Generating build information"
    VERBATIM
)

add_dependencies(
    Pixel_Storm
    Pixel_StormBuildInfo
)

target_include_directories(
    Pixel_Storm
    PRIVATE
        "${PIXEL_STORM_GENERATED_DIR}"
)
