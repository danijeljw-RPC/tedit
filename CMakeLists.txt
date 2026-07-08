cmake_minimum_required(VERSION 3.20)
project(tui_editor_starter C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(
    $<$<C_COMPILER_ID:MSVC>:/W4>
    "$<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>"
)

add_executable(tedit
    src/main.c
    src/core/document.c
    src/core/editor.c
    src/core/keymap.c
    src/core/search.c
    src/core/undo.c
    src/platform/platform.c
    src/platform/terminal_posix.c
    src/platform/terminal_win32.c
    src/renderer/ansi_renderer.c
    src/renderer/screen_buffer.c
    src/ui/editor_view.c
    src/ui/status_bar.c
)

target_include_directories(tedit PRIVATE src)

if (WIN32)
    target_compile_definitions(tedit PRIVATE TEDIT_PLATFORM_WINDOWS=1)
else()
    target_compile_definitions(tedit PRIVATE TEDIT_PLATFORM_POSIX=1)
endif()

enable_testing()

add_executable(tedit_tests
    tests/test_main.c
    src/core/document.c
    src/core/keymap.c
    src/core/search.c
    src/core/undo.c
    src/renderer/screen_buffer.c
)

target_include_directories(tedit_tests PRIVATE src)
add_test(NAME tedit_tests COMMAND tedit_tests)
