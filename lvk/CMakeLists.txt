cmake_minimum_required(VERSION 3.14)
project(lvk)

set (CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_library(${PROJECT_NAME} src/VulkanAPI.cpp 
                            src/spirv_reflect.c
                            include/VulkanAPI.h
                            include/spirv_reflect.h
                            include/VulkanMemoryAllocator.h
                            )

set(LVK_INCLUDES ${Vulkan_INCLUDE_DIRS} ${SPDLOG_INCLUDES} ${GLM_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories (${PROJECT_NAME} PUBLIC ${LVK_INCLUDES})

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} glm spdlog)
