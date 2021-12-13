cmake_minimum_required(VERSION 3.14)
project(lvk)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_executable(${PROJECT_NAME} src/main.cpp )

target_include_directories (${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})