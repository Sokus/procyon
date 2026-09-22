if(WIN32 OR LINUX)
    add_subdirectory(glfw)
endif()

if(LINUX)
    add_subdirectory(glad-gl-4.2-core)
endif()

add_subdirectory(retro-stb)
