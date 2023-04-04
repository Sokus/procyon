cmake_minimum_required(VERSION 3.5)
project(procyon)

add_executable(procyon src/main.c src/core.c src/pe_time.c)
target_link_libraries(procyon
    PUBLIC
        pspdebug pspdisplay pspge pspgu pspgum
)

create_pbp_file(
    TARGET procyon
    ICON_PATH NULL
    PREVIEW_PATH NULL
    TITLE "Procyon"
    BUILD_PRX
)