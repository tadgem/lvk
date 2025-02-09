cmake_minimum_required(VERSION 3.14)
project(example-uniforms)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} main.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${LVK_SDL_INCLUDES})

target_link_libraries(${PROJECT_NAME} lvk lvk-sdl)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)