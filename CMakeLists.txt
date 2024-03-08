cmake_minimum_required(VERSION 3.5)
project(procyon)

if (UNIX AND NOT APPLE AND NOT PSP)
    set(LINUX TRUE)
endif()
add_subdirectory(ext)

# COMPILE SHARED LIBRARY:

add_library(shared STATIC
    src/pe_core.c
    src/pe_time.c
    src/pe_random.c
    src/pe_net.c
    src/pe_bit_stream.c
    src/pe_protocol.c
    src/pe_file_io.c
    src/pe_profile.c
    src/pe_trace.c
    src/game/pe_entity.c
)
target_include_directories(shared PRIVATE src)
target_link_libraries(shared handmade-math)
if (PSP)
    target_compile_definitions(shared PUBLIC -DPE_TRACE_ENABLED)
    target_compile_options(shared PUBLIC -O3)
elseif(MSVC)
    target_compile_options(shared PUBLIC -W3 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -DPE_TRACE_ENABLED)
endif()

# COMPILE CLIENT:

add_executable(client
    src/client/main.c
    src/client/pe_graphics.c
    src/client/pe_platform.c
    src/client/pe_model.c
    src/client/pe_math.c
    src/client/pe_input.c
)
target_include_directories(client PRIVATE src)
target_link_libraries(client PRIVATE shared handmade-math psp-stb)

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
        pspdisplay pspge pspgu pspgum_vfpu pspvfpu pspctrl
        pspnet pspnet_apctl pspnet_resolver pspdebug pspgu
    )
endif()

# COMPILE SERVER:

if(WIN32 OR LINUX)
    add_executable(server src/server/main.c)
    target_include_directories(server PRIVATE src)
    target_link_libraries(server PRIVATE shared)
endif()

# COMPILE OTHER TARGETS:

#add_executable(socket_test src/samples/psp_socket_tests.c)
#target_include_directories(socket_test PRIVATE src)
#target_link_libraries(socket_test shared)

# add_executable(tests src/pe_tests.c)
# target_include_directories(tests PRIVATE src)
# target_link_libraries(tests shared)

if(WIN32 OR LINUX)
    add_executable(export_google_trace src/tools/export_google_trace.c)
    target_include_directories(export_google_trace PRIVATE src)
    target_link_libraries(export_google_trace shared)
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