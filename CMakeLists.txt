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

# CORE LIBRARY:

add_library(core STATIC
    src/core/pe_core.c
    src/core/pe_file_io.c
    src/core/pe_random.c
)

# PLATFORM LIBRARY

add_library(platform STATIC
    src/platform/pe_net.c
    src/platform/pe_time.c
)
target_link_libraries(platform PRIVATE settings core)
if(PSP)
    target_link_libraries(platform PRIVATE pspnet pspnet_apctl pspnet_resolver)
endif()

# UTILITY LIBRARY

add_library(utility STATIC src/utility/pe_trace.c)
target_link_libraries(utility PRIVATE settings core)

# GAME LIBRARY

add_library(game STATIC
    src/game/pe_bit_stream.c
    src/game/pe_entity.c
    src/game/pe_protocol.c
)
target_link_libraries(game PRIVATE settings)
target_link_libraries(game PUBLIC handmade-math)

# COMPILE CLIENT:

add_executable(client
    src/client/main.c
    src/client/pe_graphics.c
    src/client/pe_platform.c
    src/client/pe_model.c
    src/client/pe_math.c
    src/client/pe_input.c
)
target_link_libraries(client PRIVATE
    settings core platform utility game
    handmade-math psp-stb
)

if(WIN32)
    target_sources(client PRIVATE
        src/client/pe_window_glfw.c
        src/client/pe_graphics_win32.c
        src/client/pe_input_glfw.c
    )
    target_link_libraries(client PRIVATE glfw)
elseif(LINUX)
    target_sources(client PRIVATE
        src/client/pe_window_glfw.c
        ext/glad-gl-4.2-core/src/glad.c # TODO: get rid of this...
        src/client/pe_graphics_linux.c
        src/client/pe_input_glfw.c
    )
    target_link_libraries(client PRIVATE glfw)
    target_link_libraries(client PUBLIC glad)
elseif(PSP)
    target_sources(client PRIVATE
        src/client/pe_graphics_psp.c
        src/client/pe_platform_psp.c
        src/client/pe_input_psp.c
    )
    target_link_libraries(client PUBLIC
        pspdisplay pspge pspgu pspgum_vfpu pspvfpu pspctrl pspdebug
    )
endif()

# COMPILE SERVER:

if(WIN32 OR LINUX)
    add_executable(server src/server/main.c)
    target_link_libraries(server PRIVATE settings core platform game)
endif()

# COMPILE OTHER TARGETS:

#add_executable(socket_test src/samples/psp_socket_tests.c)
#target_link_libraries(socket_test shared)

# add_executable(tests src/pe_tests.c)
# target_include_directories(tests PRIVATE src)
# target_link_libraries(tests shared)

if(WIN32 OR LINUX)
    add_executable(export_google_trace src/tools/export_google_trace.c)
    target_link_libraries(export_google_trace settings core platform utility)
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