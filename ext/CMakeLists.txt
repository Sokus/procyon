add_library(handmade-math INTERFACE)
target_include_directories(handmade-math INTERFACE ${CMAKE_CURRENT_LIST_DIR}/HandmadeMath)
if(LINUX)
    target_link_libraries(handmade-math INTERFACE m)
endif()

if(NOT SERVER_STANDALONE)
    if(WIN32 OR LINUX)
        add_subdirectory(glfw)
    endif()

    if(LINUX)
        add_subdirectory(glad-gl-4.2-core)
    endif()

    add_library(psp-stb INTERFACE)
    target_include_directories(psp-stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/psp-stb)
endif()
