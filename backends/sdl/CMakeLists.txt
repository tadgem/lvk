cmake_minimum_required(VERSION 3.14)
project(lvk-sdl)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

add_library(${PROJECT_NAME} STATIC
        src/VkSDL.cpp
        include/VkSDL.h
        src/ImGui/imgui_impl_sdl3.cpp
        include/ImGui/imgui_impl_sdl3.h
)

set(LVK_SDL_INCLUDES ${LVK_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include ${SDL_INCLUDES} CACHE INTERNAL "")

target_include_directories(${PROJECT_NAME} PUBLIC ${LVK_SDL_INCLUDES})

target_link_libraries(${PROJECT_NAME} SDL3-static lvk)

