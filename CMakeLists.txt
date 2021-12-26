cmake_minimum_required(VERSION 3.14)
project(lvk)

set (CMAKE_CXX_STANDARD 17)

set(SDL_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/submod/sdl)
set(GLM_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/submod/glm)
set(SPD_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/submod/spdlog)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_executable(${PROJECT_NAME} src/main.cpp src/sdl_helpers.hpp )

target_include_directories (${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})

# sdl2
if(NOT TARGET SDL2)
    add_subdirectory("${SDL_DIRECTORY}" ${CMAKE_CURRENT_BINARY_DIR}/SDL2)
    target_link_libraries(${PROJECT_NAME} SDL2)
endif()

# glm
if(NOT TARGET glm)
    add_subdirectory("${GLM_DIRECTORY}")
    target_link_libraries(${PROJECT_NAME} glm)
endif()

# spdlog
if(NOT TARGET spdlog)
    add_subdirectory("${SPD_DIRECTORY}" ${CMAKE_CURRENT_BINARY_DIR}/spdlog)
    target_link_libraries(${PROJECT_NAME} spdlog)
endif()
