cmake_minimum_required(VERSION 3.14)
project(hello-triangle)

set (CMAKE_CXX_STANDARD 17)

set(LVK_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../lvk)
set(LVK_SDL_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../backends/sdl)

add_executable(${PROJECT_NAME}  main.cpp
                                )

target_include_directories (${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} ${LVK_SDL_DIRECTORY}/src)

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})

#lvk
if(NOT TARGET lvk)
    add_subdirectory("${LVK_DIRECTORY}" ${CMAKE_CURRENT_BINARY_DIR}/lvk)
    target_link_libraries(${PROJECT_NAME} lvk)
endif()

# sdl
if(NOT TARGET lvk-sdl)
    add_subdirectory("${LVK_SDL_DIRECTORY}" ${CMAKE_CURRENT_BINARY_DIR}/lvk-sdl)
    target_link_libraries(${PROJECT_NAME} lvk-sdl)
endif()
