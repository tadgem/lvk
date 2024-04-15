cmake_minimum_required(VERSION 3.14)
project(lvk-sdl)

set (CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_library(${PROJECT_NAME} STATIC  src/VulkanAPI_SDL.cpp 
                                    "include/VulkanAPI_SDL.h"
                                    )

message("SDL Includes")
message(${SDL_INCLUDES})

set(LVK_SDL_INCLUDES ${LVK_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include ${SDL_INCLUDES} CACHE INTERNAL "")

target_include_directories (${PROJECT_NAME} PUBLIC ${LVK_SDL_INCLUDES})

target_link_libraries (${PROJECT_NAME} SDL2-static lvk)

