cmake_minimum_required(VERSION 3.14)
project(example-model)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} main.cpp)

get_filename_component(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)


target_include_directories(${PROJECT_NAME} PUBLIC ${LVK_SDL_INCLUDES} ${EXAMPLES_DIR} ${ASSIMP_INCLUDES})

target_link_libraries(${PROJECT_NAME} lvk lvk-sdl assimp)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)