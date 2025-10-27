cmake_minimum_required(VERSION 3.20)
project(ProcessViewer LANGUAGES C CXX)

# -------------------------------------------------------------------
# Standards
# -------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------
# Basic project settings / source collection
# -------------------------------------------------------------------
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# -------------------------------------------------------------------
# Platform helper: 'windows' or 'linux' (used in paths)
# -------------------------------------------------------------------
if(WIN32)
    set(PROJ_PLATFORM "windows")
elseif(UNIX)
    set(PROJ_PLATFORM "linux")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# -------------------------------------------------------------------
# Include directories (root/include + externals)
# -------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/cef"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/concurrentqueue"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/stb"
)

# -------------------------------------------------------------------
# Preprocessor defines (global + per-platform + per-config)
# -------------------------------------------------------------------
# common defines
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    CEF_USE_SANDBOX=0
    _UNICODE
    UNICODE
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        _WIN32
    )
endif()

# Debug define per-configuration
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

# -------------------------------------------------------------------
# Compiler options: multi-processor, SDL security checks (MSVC), warning level 3 mapping
# -------------------------------------------------------------------
if(MSVC)
    # /MP for multi-core compilation, /sdl for Microsoft SDL checks, /W3 warnings
    target_compile_options(${PROJECT_NAME} PRIVATE /MP /sdl /W3)
else()
    # For GCC/Clang: try to map "level 3" -> -Wall -Wextra (common) and enable text relocation warnings
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
    # Parallel compile is controlled by the build tool (e.g., cmake --build . -- -jN)
endif()

# -------------------------------------------------------------------
# Optimization per configuration
# -------------------------------------------------------------------
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:MinSizeRel>:/O1>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-O2>
        $<$<CONFIG:MinSizeRel>:-Os>
    )
endif()

# -------------------------------------------------------------------
# Library directories (per-platform, per-config)
#   root/external/cef/lib/<platform>/<Configuration>
# We use a generator expression so Visual Studio multi-config points at the right folder.
# -------------------------------------------------------------------
# Add the platform base lib directory (CMake will expand $<CONFIG> at build time)
target_link_directories(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<BOOL:1>>:${CMAKE_CURRENT_SOURCE_DIR}/external/cef/lib/${PROJ_PLATFORM}/Debug>
    $<$<AND:$<CONFIG:Release>,$<BOOL:1>>:${CMAKE_CURRENT_SOURCE_DIR}/external/cef/lib/${PROJ_PLATFORM}/Release>
    $<$<AND:$<CONFIG:RelWithDebInfo>,$<BOOL:1>>:${CMAKE_CURRENT_SOURCE_DIR}/external/cef/lib/${PROJ_PLATFORM}/RelWithDebInfo>
    $<$<AND:$<CONFIG:MinSizeRel>,$<BOOL:1>>:${CMAKE_CURRENT_SOURCE_DIR}/external/cef/lib/${PROJ_PLATFORM}/MinSizeRel>
)

# -------------------------------------------------------------------
# Link libraries (names only, extension depends on platform)
# User asked for "just the names" (no paths) and extension varies by platform.
# We add platform-specific names with generator expressions.
# -------------------------------------------------------------------
if(WIN32)
    # explicit filenames requested (libcef.lib / libcef_dll_wrapper.lib)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:libcef.lib>
        $<$<CONFIG:Release>:libcef.lib>
        $<$<CONFIG:RelWithDebInfo>:libcef.lib>
        $<$<CONFIG:MinSizeRel>:libcef.lib>

        $<$<CONFIG:Debug>:libcef_dll_wrapper.lib>
        $<$<CONFIG:Release>:libcef_dll_wrapper.lib>
        $<$<CONFIG:RelWithDebInfo>:libcef_dll_wrapper.lib>
        $<$<CONFIG:MinSizeRel>:libcef_dll_wrapper.lib>
    )
else()
    # On Linux the library filenames are likely libcef.so (shared) or libcef.a (static).
    # Link names: -lcef style could be used, but user asked for "names" — append .so here.
    target_link_libraries(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:libcef.so>
        $<$<CONFIG:Release>:libcef.so>
        $<$<CONFIG:RelWithDebInfo>:libcef.so>
        $<$<CONFIG:MinSizeRel>:libcef.so>

        $<$<CONFIG:Debug>:libcef_dll_wrapper.so>
        $<$<CONFIG:Release>:libcef_dll_wrapper.so>
        $<$<CONFIG:RelWithDebInfo>:libcef_dll_wrapper.so>
        $<$<CONFIG:MinSizeRel>:libcef_dll_wrapper.so>
    )
endif()

# -------------------------------------------------------------------
# Post-build copying:
# 1) Copy contents of external/cef/bin/<platform>/$<CONFIG> -> output directory
# 2) Copy contents of external/cef/resources -> output directory (contents only)
# Both copy actions use generator expressions so they happen at build time and are config-aware.
# -------------------------------------------------------------------
# Copy binary runtime files (platform/config)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/external/cef/bin/${PROJ_PLATFORM}/$<CONFIG>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying CEF runtime files for platform ${PROJ_PLATFORM} config $<CONFIG> -> output"
)

# Copy cef resources (contents copied directly into output dir)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/external/cef/resources/${PROJ_PLATFORM}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying CEF resources contents -> output directory"
)

# -------------------------------------------------------------------
# Copy TypeScript build output into html/ folder in output directory
# -------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/html"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../typescript/dist"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/html"
    COMMENT "Copying TypeScript dist/ -> output/html/"
)




# -------------------------------------------------------------------
# UAC / privileged execution:
# - Windows: set manifest to requireAdministrator
# - Linux: generate a small wrapper script in the output dir that re-launches via sudo
# -------------------------------------------------------------------
if(WIN32)
    # Instruct linker to embed UAC manifest requiring administrator.
    # This is a quick approach: MSVC will embed the requested execution level.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
else()
    # Create a tiny script that will re-run the built binary with sudo if not root.
    # We generate it at configure time and copy it to the output directory at build time.
    set(RUN_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/run_with_sudo.sh")
    file(WRITE "${RUN_WRAPPER}" "#!/usr/bin/env bash\n\nBIN=\"\$1\"\nshift\nif [ \"$(id -u)\" -ne 0 ]; then\n  exec sudo \"\$BIN\" \"$@\"\nelse\n  exec \"\$BIN\" \"$@\"\nfi\n")
    execute_process(COMMAND ${CMAKE_COMMAND} -E chmod +x "${RUN_WRAPPER}")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RUN_WRAPPER}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/run_with_sudo.sh"
        COMMENT "Generating run_with_sudo.sh in output directory (will escalate with sudo if needed)"
    )
endif()

# -------------------------------------------------------------------
# Windows subsystem (no console)
# -------------------------------------------------------------------
if(WIN32)
    # Link as a Windows GUI app (no console window)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# -------------------------------------------------------------------
# Helpful target properties: RUNTIME output dir & where to place libs for multi-config
# Make sure runtime output is in a predictable location per-config.
# -------------------------------------------------------------------
# Put built binary next to the copied resources and DLLs
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)

# -------------------------------------------------------------------
# Debugger working directory (Visual Studio)
# -------------------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# -------------------------------------------------------------------
# Notes / small sanity checks
# -------------------------------------------------------------------
# Warn if expected cef bin/lib folders don't exist at configure time (not fatal).
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/cef/bin/${PROJ_PLATFORM}")
    message(WARNING "external/cef/bin/${PROJ_PLATFORM} does not exist at configure time. At build time the folder for the active configuration must exist: external/cef/bin/${PROJ_PLATFORM}/<Configuration>")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/cef/lib/${PROJ_PLATFORM}")
    message(WARNING "external/cef/lib/${PROJ_PLATFORM} does not exist at configure time. At build time the folder for the active configuration must exist: external/cef/lib/${PROJ_PLATFORM}/<Configuration>")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/cef/resources")
    message(WARNING "external/cef/resources not found; nothing to copy for resources.")
endif()

# -------------------------------------------------------------------
# End of file
# -------------------------------------------------------------------
