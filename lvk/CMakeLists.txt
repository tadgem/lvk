cmake_minimum_required(VERSION 3.14)
project(lvk)

set(CMAKE_CXX_STANDARD 17)

option(LVK_RT_SHADER_COMPILATION "Enable Runtime Shader Compilation" TRUE)

set(LVK_SRCS
    src/lvk/Texture.cpp
    src/lvk/Framebuffer.cpp
    src/lvk/Mesh.cpp
    src/lvk/Material.cpp
    src/lvk/Shader.cpp
    src/lvk/Structs.cpp
    src/lvk/DescriptorSetAllocator.cpp
    src/lvk/Pipeline.cpp
    src/lvk/Buffer.cpp
    src/lvk/Commands.cpp
    src/lvk/Utils.cpp
    src/lvk/Init.cpp
    src/lvk/Descriptor.cpp
    src/lvk/RenderPass.cpp
    src/ThirdParty/spirv_reflect.c
    src/ImGui/imgui_impl_vulkan.cpp
    src/ImGui/imgui_draw.cpp
    src/ImGui/imgui_tables.cpp
    src/ImGui/imgui_widgets.cpp
    src/ImGui/imgui.cpp
    src/Im3D/im3d.cpp
    src/Im3D/im3d_lvk.cpp
    include/lvk/lvk.h
    include/lvk/Texture.h
    include/lvk/Framebuffer.h
    include/lvk/Mesh.h
    include/lvk/Material.h
    include/lvk/Shader.h
    include/lvk/Macros.h
    include/lvk/DescriptorSetAllocator.h
    include/lvk/Pipeline.h
    include/lvk/Structs.h
    include/lvk/Buffer.h
    include/lvk/Commands.h
    include/lvk/Utils.h
    include/lvk/Init.h
    include/lvk/Descriptor.h
    include/lvk/RenderPass.h
    include/Alias.h
    include/ThirdParty/spirv_reflect.h
    include/ThirdParty/VulkanMemoryAllocator.h
    include/ThirdParty/stb_image.h
    include/Im3D/im3d.h
    include/Im3D/im3d_math.h
    include/Im3D/im3d_config.h
    include/Im3D/im3d_lvk.h
    include/ImGui/imgui_impl_vulkan.h
    include/ImGui/lvk_extensions.h
    include/ImGui/imconfig.h
    include/ImGui/imgui_internal.h
    include/ImGui/imgui.h
    include/ImGui/imstb_rectpack.h
    include/ImGui/imstb_textedit.h
    include/ImGui/imstb_truetype.h
)

set(LVK_COMPILE_DEFS "/DIMGUI_IMPL_VULKAN_USE_VOLK")

if(LVK_RT_SHADER_COMPILATION)
    set(LVK_SRCS ${LVK_SRCS} src/lvk/ShaderCompilation.cpp)
    set(LVK_COMPILE_DEFS ${LVK_COMPILE_DEFS} "/DLVK_RT_SHADER_COMPILATION")
endif()

add_library(${PROJECT_NAME} STATIC
    ${LVK_SRCS}
)

set(LVK_INCLUDES    ${Vulkan_INCLUDE_DIRS}
                    ${SPDLOG_INCLUDES}
                    ${GLM_INCLUDES}
                    ${SHADERC_INCLUDES}
                    ${SHADERC_UTIL_INCLUDES}
                    ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")
target_include_directories(${PROJECT_NAME} PUBLIC ${LVK_INCLUDES})

if (MSVC)
    set(${LVK_COMPILE_DEFS} ${LVK_COMPILE_DEFS} "/bigobj" "/MP" "/Zi")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,Release>:ProgramDatabase>")
endif ()

target_compile_options(${PROJECT_NAME} PRIVATE ${LVK_COMPILE_DEFS})

target_link_libraries(${PROJECT_NAME} volk spdlog shaderc shaderc_util)
