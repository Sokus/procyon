cmake_minimum_required(VERSION 3.5)
project(procyon)

if(PSP)
add_executable(procyon
    src/client/psp_main.c
    src/pe_core.c
    src/pe_time.c
    src/pe_input.c
)
endif()

if(WIN32)
add_executable(procyon
    src/client/win32_main.c
    src/pe_core.c
    src/pe_time.c
)
endif()

target_include_directories(procyon PRIVATE src)

if(PSP)
    target_link_libraries(procyon
        PUBLIC
            pspdebug pspdisplay pspge pspgu pspgum pspctrl
    )

    create_pbp_file(
        TARGET procyon
        ICON_PATH NULL
        PREVIEW_PATH NULL
        TITLE "Procyon"
        BUILD_PRX
    )
endif()