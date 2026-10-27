cmake_minimum_required(VERSION 3.16)
project(PomodoroApp VERSION 1.0.0 LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# SDL2: MSVC uses vendor/SDL2 (VC prebuilt), MinGW uses
# vendor/SDL2-2.32.10/x86_64-w64-mingw32. Both are vendored in this repo.
# ---------------------------------------------------------------------------
if(MSVC)
    set(SDL2_ROOT "${CMAKE_SOURCE_DIR}/vendor/SDL2")
    set(SDL2_INCLUDE_DIR "${SDL2_ROOT}/include")
    set(SDL2_LIB_DIR "${SDL2_ROOT}/lib/x64")
    set(SDL2_LIBS SDL2main SDL2)
    set(SDL2_DLL "${SDL2_LIB_DIR}/SDL2.dll")
else()
    set(SDL2_ROOT "${CMAKE_SOURCE_DIR}/vendor/SDL2-2.32.10/x86_64-w64-mingw32")
    set(SDL2_INCLUDE_DIR "${SDL2_ROOT}/include/SDL2")
    set(SDL2_LIB_DIR "${SDL2_ROOT}/lib")
    # Order matters for MinGW: mingw32 + SDL2main provide WinMain wrapper.
    set(SDL2_LIBS mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8
                  user32 gdi32 winmm imm32 ole32 oleaut32 shell32
                  setupapi version uuid)
    set(SDL2_DLL "${SDL2_ROOT}/bin/SDL2.dll")
endif()

# ---------------------------------------------------------------------------
# Dear ImGui sources (core + SDL2 + OpenGL3 backends)
# ---------------------------------------------------------------------------
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/vendor/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(APP_SOURCES
    src/main.cpp
    src/app.cpp
    src/timer.cpp
    src/gif_player.cpp
    src/hotkey.cpp
    src/ui.cpp
    src/settings.cpp
    ${IMGUI_SOURCES}
    resources/app.rc
)

add_executable(pomodoro WIN32 ${APP_SOURCES})

target_include_directories(pomodoro PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/vendor/stb
    ${SDL2_INCLUDE_DIR}
)

target_link_directories(pomodoro PRIVATE ${SDL2_LIB_DIR})
target_link_libraries(pomodoro PRIVATE ${SDL2_LIBS} opengl32)

if(MSVC)
    target_compile_options(pomodoro PRIVATE /utf-8 /W3)
    target_link_libraries(pomodoro PRIVATE winmm imm32 version)
else()
    target_compile_options(pomodoro PRIVATE -Wall -Wextra)
    # Fully static MinGW runtimes so the .exe runs on any Windows machine
    # with just SDL2.dll next to it. SDL2 itself stays dynamic (import lib).
    target_link_options(pomodoro PRIVATE -static)
endif()

# Single .exe, no console window (M7)
set_target_properties(pomodoro PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "Pomodoro"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Bundle SDL2.dll + GIF assets next to the .exe (M7)
add_custom_command(TARGET pomodoro POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DLL}" "$<TARGET_FILE_DIR:pomodoro>/SDL2.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:pomodoro>/assets"
    COMMENT "Bundling SDL2.dll and assets next to Pomodoro.exe"
)
