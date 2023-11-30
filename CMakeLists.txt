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

# COMPILE CLIENT LIBRARY:

if(NOT SERVER_STANDALONE)
    add_library(client STATIC
        src/client/pe_client.c
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
        target_link_libraries(client PRIVATE glfw glad)
    elseif(PSP)
        target_sources(client PRIVATE
            src/client/pe_graphics_psp.c
            src/client/pe_platform_psp.c
            src/client/pe_input_psp.c
        )
        target_link_libraries(client PUBLIC
            pspdebug pspdisplay pspge pspgu pspgum pspctrl
            pspnet pspnet_apctl pspnet_resolver
        )
    endif()
endif()

# COMPILE MAIN TARGET:

if(NOT SERVER_STANDALONE)
    set(main_target procyon)
else()
    set(main_target server)
endif()
add_executable(${main_target}
    src/pe_tests.c
)
target_include_directories(${main_target} PRIVATE src)
target_link_libraries(${main_target} PRIVATE shared)
if(NOT SERVER_STANDALONE)
    target_link_libraries(${main_target} PRIVATE client)
    if(PSP)
        create_pbp_file(
            TARGET ${main_target}
            ICON_PATH NULL
            PREVIEW_PATH NULL
            TITLE "Procyon"
            BUILD_PRX
        )
    endif()
else()
    target_compile_definitions(${main_target} PRIVATE PE_SERVER_STANDALONE)
endif()

# COMPILE OTHER TARGETS:

add_executable(tests src/pe_tests.c)
target_include_directories(tests PRIVATE src)
target_link_libraries(tests shared client)