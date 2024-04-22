cmake_minimum_required(VERSION 3.14)
project(lvk-all)

set(SDL_STATIC TRUE CACHE INTERNAL TRUE)
# remove ZERO_CHECK
set(CMAKE_SUPPRESS_REGENERATION true)
# remove INSTALL 
set(CMAKE_SKIP_INSTALL_RULES true)
set(SDL2_DISABLE_SDL2MAIN       ON CACHE BOOL "Disable SDL Entrypoint")
set(SDL2_DISABLE_INSTALL        ON CACHE BOOL "Disable SDL Install")
set(SDL2_DISABLE_UNINSTALL      ON CACHE BOOL "Disable SDL Uninstall")

add_subdirectory(submodules/glm)
add_subdirectory(submodules/spdlog)
add_subdirectory(third-party/sdl)

set(GLM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/submodules/glm CACHE INTERNAL "")
set(SPDLOG_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/submodules/spdlog/include CACHE INTERNAL "")
set(SDL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/third-party/sdl/include CACHE INTERNAL "")

set_target_properties(glm PROPERTIES FOLDER "ThirdParty/glm")
set_target_properties(sdl_headers_copy PROPERTIES FOLDER "ThirdParty/SDL")
set_target_properties(SDL2 PROPERTIES FOLDER "ThirdParty/SDL")
set_target_properties(SDL2_test PROPERTIES FOLDER "ThirdParty/SDL")
set_target_properties(SDL2-static PROPERTIES FOLDER "ThirdParty/SDL")
set_target_properties(spdlog PROPERTIES FOLDER "ThirdParty/spdlog")

add_subdirectory(lvk)
add_subdirectory(backends/sdl)
add_subdirectory(examples/hello-triangle)
add_subdirectory(examples/vertex-index-buffer)
add_subdirectory(examples/uniforms)

