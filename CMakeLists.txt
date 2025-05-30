cmake_minimum_required(VERSION 3.5)
project(procyon)

if (UNIX AND NOT APPLE AND NOT PSP)
    set(LINUX TRUE)
endif()
add_subdirectory(ext)

# PROJECT-WIDE TARGET SETTINGS

add_library(settings INTERFACE)
target_include_directories(settings INTERFACE src)
if(MSVC)
    target_compile_options(settings INTERFACE -W3 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127)
endif()
target_compile_definitions(settings INTERFACE P_TRACE_ENABLED)

# CORE LIBRARY:

add_library(core STATIC
    src/core/p_core.c
)

# MATH LIBRARY:

add_library(math INTERFACE)
target_link_libraries(math INTERFACE handmade-math)

if(PSP)
    target_link_libraries(math INTERFACE pspgum_vfpu pspvfpu)
endif()

# PLATFORM LIBRARY

add_library(platform STATIC
    src/platform/p_input.c
    src/platform/p_net.c
    src/platform/p_window.c
)
target_link_libraries(platform PRIVATE settings core)
target_link_libraries(platform PRIVATE graphics) # TODO: remove this
if(WIN32)
    target_sources(platform PRIVATE
        src/platform/p_file_win32.c
        src/platform/p_window_glfw.c
        src/platform/p_input_glfw.c
    )
    target_link_libraries(platform PRIVATE glfw)
elseif(LINUX)
    target_sources(platform PRIVATE
        src/platform/p_file_linux.c
        src/platform/p_window_glfw.c
        src/platform/p_input_glfw.c
    )
    target_link_libraries(platform PRIVATE glfw)
    target_link_libraries(platform PUBLIC glad)
elseif(PSP)
    target_sources(platform PRIVATE
        src/platform/p_file_psp.c
        src/platform/p_window_psp.c
        src/platform/p_input_psp.c
    )
    target_link_libraries(platform PUBLIC
        pspnet pspnet_apctl pspnet_resolver
    )
endif()

# GRAPHICS LIBRARY

add_library(graphics STATIC
    src/graphics/p_graphics.c
    src/graphics/p_model.c
    src/graphics/p_graphics_math.c
    src/graphics/p_pbm.c
)
target_link_libraries(graphics PRIVATE
    settings core math
    psp-stb
)
target_link_libraries(graphics PUBLIC platform math)
if(WIN32)
    target_sources(graphics PRIVATE
        src/graphics/p_graphics_win32.c
        src/graphics/p_model_win32.c
        src/graphics/p_asset_loader.c
    )
elseif(LINUX)
    target_sources(graphics PRIVATE
        ext/glad-gl-4.2-core/src/glad.c # TODO: get rid of this...
        src/graphics/p_graphics_linux.c
        src/graphics/p_model_linux.c
    )
    target_link_libraries(graphics PUBLIC glad)
elseif(PSP)
    target_sources(graphics PRIVATE
        src/graphics/p_graphics_psp.c
        src/graphics/p_model_psp.c
    )
    target_link_libraries(graphics PUBLIC
        pspdisplay pspge pspgu pspgum_vfpu pspvfpu pspctrl pspdebug
    )
endif()

# UTILITY LIBRARY

add_library(utility STATIC src/utility/p_trace.c)
target_link_libraries(utility PRIVATE settings core)

# GAME LIBRARY

add_library(game STATIC
    src/game/p_bit_stream.c
    src/game/p_entity.c
    src/game/p_protocol.c
)
target_link_libraries(game PRIVATE settings core math)

# COMPILE CLIENT:

add_executable(client
    src/client/main.c
)
target_link_libraries(client PRIVATE
    settings core math platform graphics utility game
    psp-stb
)

# COMPILE SERVER:

if(WIN32 OR LINUX)
    add_executable(server src/server/main.c)
    target_link_libraries(server PRIVATE settings core math platform game)
endif()

# COMPILE OTHER TARGETS:

# add_executable(tests src/samples/p_tests.c)
# target_link_libraries(tests PRIVATE settings core platform graphics utility)

if(WIN32 OR LINUX)
    add_executable(export_google_trace src/tools/export_google_trace.c)
    target_link_libraries(export_google_trace settings core platform utility)

    add_executable(tests tests/test_main.c)
    target_link_libraries(tests settings core utility)
endif()

# ------------------------------------------------------------

SET(PSP_TARGET "client" CACHE STRING "Target to build the PBP from")
if(PSP)
    if(TARGET ${PSP_TARGET})
        create_pbp_file(
            TARGET ${PSP_TARGET}
            ICON_PATH NULL
            PREVIEW_PATH NULL
            TITLE "Procyon"
            BUILD_PRX
        )
    else()
        message(SEND_ERROR "PSP: target ${PSP_TARGET} doesn't exist")
    endif()
endif()
