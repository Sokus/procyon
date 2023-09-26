cmake_minimum_required(VERSION 3.5)
project(procyon)

if (UNIX AND NOT APPLE AND NOT PSP)
    set(LINUX TRUE)
endif()
add_subdirectory(ext)

# COMPILE SHARED LIBRARY:

add_library(shared STATIC
    src/pe_core.c
    src/pe_string.c
    src/pe_time.c
    src/pe_net.c
    src/pe_bit_stream.c
    src/pe_protocol.c
    src/pe_file_io.c
    src/pe_temp_arena.c
    src/game/pe_entity.c
)
target_include_directories(shared PRIVATE src)
target_link_libraries(shared handmade-math)
if (PSP)
    target_compile_options(shared PUBLIC -g -O0)
elseif(MSVC)
    target_compile_options(shared PUBLIC -W3 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127)
endif()

# COMPILE MAIN TARGET:

add_executable(procyon
    src/client/main.c
    src/client/pe_graphics.c
    src/client/pe_platform.c
    src/client/pe_model.c
    src/client/pe_math.c
)
target_include_directories(procyon PRIVATE src)
target_link_libraries(procyon PRIVATE shared handmade-math psp-stb)

if(WIN32)
    target_sources(procyon PRIVATE
        #src/client/win32_main.c
        src/client/pe_window_glfw.c
        src/client/pe_graphics_win32.c
    )
    target_link_libraries(procyon PRIVATE glfw)
elseif(LINUX)
    target_sources(procyon PRIVATE
        #src/client/linux_main.c
        src/client/pe_window_glfw.c
        ext/glad-gl-4.2-core/src/glad.c
        src/client/pe_graphics_linux.c
    )
    target_link_libraries(procyon PRIVATE glfw glad)
elseif(PSP)
    target_sources(procyon PRIVATE
        #src/client/psp_main.c
        src/client/pe_graphics_psp.c
        src/client/pe_platform_psp.c
        src/client/pe_input.c
    )
    target_link_libraries(procyon PUBLIC
        pspdebug pspdisplay pspge pspgu pspgum pspctrl
        pspnet pspnet_apctl pspnet_resolver
    )
    create_pbp_file(
        TARGET procyon
        ICON_PATH NULL
        PREVIEW_PATH NULL
        TITLE "Procyon"
        BUILD_PRX
    )
endif()

# COMPILE SERVER TARGET:

if(WIN32 OR LINUX)
    add_executable(server
        src/server/win32_main.c
    )
    target_include_directories(server PRIVATE src)
    target_link_libraries(server shared)
endif()

# COMPILE OTHER TARGETS:

add_executable(tests src/pe_tests.c)
target_include_directories(tests PRIVATE src)
target_link_libraries(tests shared)