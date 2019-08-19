cmake_minimum_required(VERSION 3.5)
project(visualization.shadertoy)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(p8-platform REQUIRED)

add_subdirectory(lib/kissfft)

if(NOT WIN32)
  if(APP_RENDER_SYSTEM STREQUAL "gl" OR NOT APP_RENDER_SYSTEM)
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

  include_directories(${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways
                      ${PROJECT_SOURCE_DIR}/lib/kissfft)

  if(${CORE_SYSTEM_NAME} STREQUAL "osx" OR
     ${CORE_SYSTEM_NAME} STREQUAL "ios" OR
     ${CORE_SYSTEM_NAME} STREQUAL "darwin_embedded")
    list(APPEND DEPLIBS "-framework CoreVideo")
  endif()

  set(SHADERTOY_SOURCES src/lodepng.cpp
                        src/main.cpp)

  set(SHADERTOY_HEADERS src/lodepng.h)

elseif(WIN32)
  include_directories(${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)
                      ${p8-platform_INCLUDE_DIRS}
                      ${PROJECT_SOURCE_DIR}/lib/kissfft
                      ${CMAKE_INCLUDE_PATH})

  set(SHADERTOY_SOURCES src/WICTextureLoader.cpp
                        src/maindx.cpp)

  set(SHADERTOY_HEADERS src/lodepng.h
                        src/WICTextureLoader.h)
endif()

list(APPEND DEPLIBS kissfft)

build_addon(visualization.shadertoy SHADERTOY DEPLIBS)

include(CPack)
