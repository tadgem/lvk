cmake_minimum_required(VERSION 3.14)
project(lvk)

set (CMAKE_CXX_STANDARD 17)

set(GLM_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../submod/glm)
set(SPD_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../submod/spdlog)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_library(${PROJECT_NAME} src/VulkanAPI.cpp 
                            src/VulkanAPI.h
                            )

target_include_directories (${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})

# glm
if(NOT TARGET glm)
    add_subdirectory("${GLM_DIRECTORY}" ${CMAKE_CURRENT_BINARY_DIR}/glm)
    target_link_libraries(${PROJECT_NAME} glm)
endif()

# spdlog
if(NOT TARGET spdlog)
    add_subdirectory("${SPD_DIRECTORY}" ${CMAKE_CURRENT_BINARY_DIR}/spdlog)
    target_link_libraries(${PROJECT_NAME} spdlog)
endif()
