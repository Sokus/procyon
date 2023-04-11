if(WIN32)
    add_subdirectory(glfw)
endif()

add_library(handmade-math INTERFACE)
target_include_directories(handmade-math INTERFACE ${CMAKE_CURRENT_LIST_DIR}/HandmadeMath)