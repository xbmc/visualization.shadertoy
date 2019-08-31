cmake_minimum_required(VERSION 3.5)
project(visualization.shadertoy)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(lib/kissfft)

if(NOT WIN32 AND (APP_RENDER_SYSTEM STREQUAL "gl" OR NOT APP_RENDER_SYSTEM))
  find_package(OpenGl REQUIRED)
  set(DEPLIBS ${OPENGL_LIBRARIES})
  set(includes ${OPENGL_INCLUDE_DIR})
  add_definitions(${OPENGL_DEFINITIONS})
else()
  find_package(OpenGLES REQUIRED)
  set(DEPLIBS ${OPENGLES_LIBRARIES})
  set(includes ${OPENGLES_INCLUDE_DIR})
  add_definitions(${OPENGLES_DEFINITIONS})
endif()

include_directories(${GLM_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways
                    ${PROJECT_SOURCE_DIR}/lib)

if(CORE_SYSTEM_NAME STREQUAL osx OR
   CORE_SYSTEM_NAME STREQUAL ios OR
   CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  list(APPEND DEPLIBS "-framework CoreVideo")
endif()

set(SHADERTOY_SOURCES src/lodepng.cpp
                      src/main.cpp)

set(SHADERTOY_HEADERS src/lodepng.h
                      src/main.h)

list(APPEND DEPLIBS kissfft)

build_addon(visualization.shadertoy SHADERTOY DEPLIBS)

include(CPack)
