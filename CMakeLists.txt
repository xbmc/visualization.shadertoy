project(visualization.shadertoy)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(p8-platform REQUIRED)

add_subdirectory(lib/kissfft)

set(DEPLIBS kissfft)

if (NOT WIN32)
  if(APPLE)
    set(DEPLIBS ${DEPLIBS} "-framework CoreVideo")
  endif()

  add_options(CXX ALL_BUILDS -std=c++11)
  find_package(OpenGL)
  if(OPENGL_FOUND)
    find_package(GLEW REQUIRED)
    set(INCLUDES ${OpenGL_INCLUDE_DIR}
                 ${GLEW_INCLUDE_DIR})
    add_definitions(-DHAS_OPENGL)
    set(DEPLIBS ${DEPLIBS} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
  else()
    find_package(OpenGLES2)
    if(OPENGLES2_FOUND)
      set(INCLUDES ${OPENGLES2_INCLUDE_DIR})
      add_definitions(-DHAS_GLES2 -DHAS_GLES ${OpenGLES2_CFLAGS_OTHER})
      set(DEPLIBS ${DEPLIBS} ${OPENGLES2_LIBRARIES})
    else()
      message(FATAL_ERROR "No rendering API found. Bailing")
    endif()
  endif()

  include_directories(${OpenGL_INCLUDE_DIR}
                      ${GLEW_INCLUDE_DIR}
                      ${KODI_INCLUDE_DIR}
                      ${p8-platform_INCLUDE_DIRS}
                      ${PROJECT_SOURCE_DIR}/lib/kissfft
                      ${CMAKE_INCLUDE_PATH})

  set(SHADERTOY_SOURCES src/lodepng.cpp
                        src/main.cpp)

  set(DEPLIBS ${DEPLIBS} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})

elseif(WIN32)
  include_directories(${KODI_INCLUDE_DIR}
                      ${p8-platform_INCLUDE_DIRS}
                      ${PROJECT_SOURCE_DIR}/lib/kissfft
                      ${CMAKE_INCLUDE_PATH})

  set(SHADERTOY_SOURCES src/WICTextureLoader.cpp
                        src/maindx.cpp)
endif()

build_addon(visualization.shadertoy SHADERTOY DEPLIBS)

include(CPack)
