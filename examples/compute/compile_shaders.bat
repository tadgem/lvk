%VULKAN_SDK%/bin/glslc.exe shaders/gbuffer.vert -o shaders/gbuffer.vert.spv
%VULKAN_SDK%/bin/glslc.exe shaders/gbuffer.frag -o shaders/gbuffer.frag.spv

%VULKAN_SDK%/bin/glslc.exe shaders/lights.vert -o shaders/lights.vert.spv
%VULKAN_SDK%/bin/glslc.exe shaders/lights.frag -o shaders/lights.frag.spv

%VULKAN_SDK%/bin/glslc.exe shaders/particles.comp -o shaders/particles.comp.spv