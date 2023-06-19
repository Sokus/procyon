if(WIN32)
    add_subdirectory(glfw)
endif()

add_library(handmade-math INTERFACE)
target_include_directories(handmade-math INTERFACE ${CMAKE_CURRENT_LIST_DIR}/HandmadeMath)

add_library(model3d INTERFACE)
target_include_directories(model3d INTERFACE ${CMAKE_CURRENT_LIST_DIR}/model3d)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/stb)