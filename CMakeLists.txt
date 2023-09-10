cmake_minimum_required(VERSION 3.5)
project(procyon)

if (UNIX AND NOT APPLE AND NOT PSP)
    set(LINUX TRUE)
endif()

add_subdirectory(ext)

add_library(shared STATIC
    src/pe_core.c
    src/pe_time.c
    src/pe_net.c
    src/pe_bit_stream.c
    src/pe_protocol.c
    src/pe_file_io.c
    src/pe_temp_allocator.c
    src/game/pe_entity.c
)
target_include_directories(shared PRIVATE src)
target_link_libraries(shared handmade-math)
if (PSP)
    target_compile_options(shared PRIVATE -g -O0)
endif()

if(WIN32 OR LINUX)
    add_executable(procyon
        #src/client/main.c
        src/client/linux_main.c
        src/client/pe_graphics.c
        src/client/pe_platform.c
        src/client/pe_model.c
        src/client/pe_window_glfw.c
    )

    if(WIN32)
        target_sources(procyon PRIVATE
            #src/client/win32_main.c
            #src/client/win32_net_sample.c
            src/client/win32/win32_d3d.c
            src/client/win32/win32_shader.c
        )
    elseif(LINUX)
        target_sources(procyon PRIVATE
            ext/glad-gl-4.2-core/src/glad.c
            src/client/pe_graphics_linux.c
        )
    endif()

    target_include_directories(procyon PRIVATE src)
    target_link_libraries(procyon shared glfw handmade-math psp-stb)
    if(LINUX)
        target_link_libraries(procyon glad)
    endif()

    add_executable(server
        src/server/win32_main.c
    )
    target_include_directories(server PRIVATE src)
    target_link_libraries(server shared)

    if(MSVC)
        add_compile_options(-WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127)
    endif()
endif()

if (PSP)
    add_executable(procyon
        #src/client/main.c
        src/client/psp_main.c
        #src/client/psp_net_sample.c
        src/client/pe_graphics.c
        src/client/pe_graphics_psp.c
        src/client/pe_platform.c
        src/client/pe_platform_psp.c
        src/client/pe_model.c
        src/client/pe_input.c
    )
    target_compile_options(procyon PRIVATE -g -O0)
    target_include_directories(procyon PRIVATE src)
    target_link_libraries(procyon
        PUBLIC
            shared handmade-math psp-stb
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
