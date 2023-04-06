cmake_minimum_required(VERSION 3.5)
project(procyon)

add_library(shared STATIC
    src/pe_core.c
    src/pe_time.c
    src/pe_net.c
)
target_include_directories(shared PRIVATE src)

if(PSP)
    add_executable(procyon
        #src/client/psp_main.c
        src/client/psp_net_sample.c
        src/pe_input.c
    )
elseif(WIN32)
    add_executable(procyon
        src/client/win32_main.c
    )
    add_executable(server
        src/server/win32_main.c
    )
    target_link_libraries(procyon shared)
    target_include_directories(server PRIVATE src)
    target_link_libraries(server shared)
endif()

target_include_directories(procyon PRIVATE src)

if(PSP)
    target_link_libraries(procyon
        PUBLIC
            shared
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