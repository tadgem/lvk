cmake_minimum_required(VERSION 3.14)
project(lvk)

set(SDL_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/submod/sdl)
set(GLM_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/submod/glm)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_executable(${PROJECT_NAME} src/main.cpp )

target_include_directories (${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})

# sdl2
if(NOT TARGET SDL2)
    add_subdirectory("${SDL_DIRECTORY}")
    target_link_libraries(${PROJECT_NAME} SDL2)
endif()

# glm
if(NOT TARGET glm)
    add_subdirectory("${GLM_DIRECTORY}")
    target_link_libraries(${PROJECT_NAME} glm)
endif()
