project(visualization.shadertoy)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(kodi REQUIRED)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${OpenGL_INCLUDE_DIR}
                    ${GLEW_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/kissfft)

add_subdirectory(lib/kissfft)

set(SHADERTOY_SOURCES src/lodepng.cpp
                      src/main.cpp)

set(DEPLIBS ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} kissfft)

add_options(CXX ALL_BUILDS -std=c++11)

build_addon(visualization.shadertoy SHADERTOY DEPLIBS)

include(CPack)
