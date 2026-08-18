set(BUILD_GIT_BRANCH "unknown")
set(BUILD_GIT_HASH "unknown")
set(BUILD_GIT_DIRTY false)

if(GIT_EXECUTABLE)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_BRANCH_RESULT
        ERROR_QUIET
    )

    if(GIT_BRANCH_RESULT EQUAL 0 AND NOT GIT_BRANCH STREQUAL "")
        set(BUILD_GIT_BRANCH "${GIT_BRANCH}")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" rev-parse --short=8 HEAD
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_HASH_RESULT
        ERROR_QUIET
    )

    if(GIT_HASH_RESULT EQUAL 0 AND NOT GIT_HASH STREQUAL "")
        set(BUILD_GIT_HASH "${GIT_HASH}")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" status --porcelain
        OUTPUT_VARIABLE GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_STATUS_RESULT
        ERROR_QUIET
    )

    if(GIT_STATUS_RESULT EQUAL 0 AND NOT GIT_STATUS STREQUAL "")
        set(BUILD_GIT_DIRTY true)
    endif()
endif()

get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

configure_file(
    "${TEMPLATE_FILE}"
    "${OUTPUT_FILE}"
    @ONLY
)
