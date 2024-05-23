cmake_minimum_required(VERSION 3.14)
project(lvk)

set (CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

add_library(${PROJECT_NAME} STATIC src/lvk/VulkanAPI.cpp 
                            src/spirv_reflect.c
                            src/ImGui/imgui_impl_vulkan.cpp
                            src/ImGui/imgui_draw.cpp
                            src/ImGui/imgui_tables.cpp
                            src/ImGui/imgui_widgets.cpp
                            src/ImGui/imgui.cpp
                            include/lvk/VulkanAPI.h
                            include/lvk/Texture.h
                            include/lvk/Framebuffer.h
                            include/Alias.h
                            include/spirv_reflect.h
                            include/VulkanMemoryAllocator.h
                            include/stb_image.h
                            include/ImGui/imgui_impl_vulkan.h
                            include/ImGui/imconfig.h
                            include/ImGui/imgui_internal.h
                            include/ImGui/imgui.h
                            include/ImGui/imstb_rectpack.h
                            include/ImGui/imstb_textedit.h
                            include/ImGui/imstb_truetype.h
                            )

set(LVK_INCLUDES ${Vulkan_INCLUDE_DIRS} ${SPDLOG_INCLUDES} ${GLM_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories (${PROJECT_NAME} PUBLIC ${LVK_INCLUDES})

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} spdlog)
