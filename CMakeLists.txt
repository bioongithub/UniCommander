cmake_minimum_required(VERSION 3.20)
project(UniCommander VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/main.cpp
    src/ui/base_window.cpp
    src/ui/directory_panel.cpp
    src/ui/help_window.cpp
    src/test_runner.cpp
)

# ── Platform-specific sources and libraries ─────────────────────────────────
if(WIN32)
    list(APPEND SOURCES
        src/platform/windows/win32_window.cpp
    )
    add_executable(unicommander WIN32 ${SOURCES})
    # WIN32 subsystem but keep standard main() as entry point
    set_target_properties(unicommander PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
    target_link_libraries(unicommander PRIVATE user32 gdi32)

elseif(APPLE)
    enable_language(OBJCXX)
    list(APPEND SOURCES
        src/platform/macos/cocoa_window.mm
    )
    add_executable(unicommander ${SOURCES})
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    target_link_libraries(unicommander PRIVATE ${COCOA_LIBRARY})

elseif(UNIX)
    list(APPEND SOURCES
        src/platform/linux/x11_window.cpp
    )
    add_executable(unicommander ${SOURCES})
    find_package(X11 REQUIRED)
    target_link_libraries(unicommander PRIVATE X11::X11)
endif()

# ── Include paths ───────────────────────────────────────────────────────────
target_include_directories(unicommander PRIVATE
    include
    src/platform/windows
    src/platform/macos
    src/platform/linux
)

# ── Compiler warnings ───────────────────────────────────────────────────────
target_compile_options(unicommander PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)
