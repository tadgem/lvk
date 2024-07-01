cmake_minimum_required(VERSION 3.14)
project(example-ssgi)

set (CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} main.cpp)

get_filename_component(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)


target_include_directories (${PROJECT_NAME} PUBLIC ${LVK_SDL_INCLUDES} ${EXAMPLES_DIR} ${ASSIMP_INCLUDES})

target_link_libraries (${PROJECT_NAME} lvk lvk-sdl assimp)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})