cmake_minimum_required(VERSION 3.14)
project(vertex-index-buffer)

set (CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME}  main.cpp
                                )

target_include_directories (${PROJECT_NAME} PUBLIC ${LVK_SDL_INCLUDES} )

target_link_libraries (${PROJECT_NAME} lvk lvk-sdl)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})