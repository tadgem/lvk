cmake_minimum_required(VERSION 3.14)
project(lvk-all)

set(SDL_STATIC TRUE CACHE INTERNAL TRUE)

add_subdirectory(submodules/glm)
add_subdirectory(submodules/spdlog)
add_subdirectory(third-party/sdl)

set(GLM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/submodules/glm CACHE INTERNAL "")
set(SPDLOG_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/submodules/spdlog/include CACHE INTERNAL "")
set(SDL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/third-party/sdl/include CACHE INTERNAL "")

add_subdirectory(lvk)
add_subdirectory(backends/sdl)
add_subdirectory(examples/hello-triangle)