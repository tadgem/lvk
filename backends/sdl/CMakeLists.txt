cmake_minimum_required(VERSION 3.14)
project(lvk-sdl)

set (CMAKE_CXX_STANDARD 17)

set(SDL_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../submod/sdl)
set(SPD_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../submod/spdlog)
set(LVK_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../lvk)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_library(${PROJECT_NAME} STATIC  src/VulkanAPI_SDL.cpp 
                                    src/VulkanAPI_SDL.h
                                    )

target_include_directories (${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} ${LVK_DIRECTORY}/src ${SPD_DIRECTORY}/include)

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})


# sdl2
if(NOT TARGET SDL2)
    add_subdirectory("${SDL_DIRECTORY}" ${CMAKE_CURRENT_BINARY_DIR}/SDL2)
    target_link_libraries(${PROJECT_NAME} SDL2)
endif()

if(NOT TARGET lvk)
    add_subdirectory("${LVK_DIRECTORY}" ${CMAKE_CURRENT_BINARY_DIR}/lvk)
    target_link_libraries(${PROJECT_NAME} lvk)
endif()