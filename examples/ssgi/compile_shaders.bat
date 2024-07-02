%VULKAN_SDK%/bin/glslc.exe shaders/gbuffer.vert -o shaders/gbuffer.vert.spv
%VULKAN_SDK%/bin/glslc.exe shaders/gbuffer.frag -o shaders/gbuffer.frag.spv

%VULKAN_SDK%/bin/glslc.exe shaders/lights.vert -o shaders/lights.vert.spv
%VULKAN_SDK%/bin/glslc.exe shaders/lights.frag -o shaders/lights.frag.spv

%VULKAN_SDK%/bin/glslc.exe shaders/SSGI.frag -o shaders/ssgi.frag.spv

%VULKAN_SDK%/bin/glslc.exe shaders/accumulate.frag -o shaders/accumulate.frag.spv
