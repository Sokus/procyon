cmake_minimum_required(VERSION 3.5)
project(procyon)

add_executable(psp_app src/main.c)
target_link_libraries(psp_app
    PUBLIC
        pspdebug pspdisplay pspge pspgu pspgum
)

create_pbp_file(
    TARGET psp_app
    ICON_PATH NULL
    PREVIEW_PATH NULL
    TITLE "Procyon App"
    BUILD_PRX
)