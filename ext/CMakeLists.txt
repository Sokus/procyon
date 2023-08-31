if(WIN32)
    add_subdirectory(glfw)
endif()

add_library(handmade-math INTERFACE)
target_include_directories(handmade-math INTERFACE ${CMAKE_CURRENT_LIST_DIR}/HandmadeMath)

add_library(psp-stb INTERFACE)
target_include_directories(psp-stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/psp-stb)

add_subdirectory(glad-gl-4.2-core)