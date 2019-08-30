cmake_minimum_required(VERSION 3.8)
project(angle)

message(STATUS "Used CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Used CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Used CMAKE_HOST_SYSTEM_VERSION: ${CMAKE_HOST_SYSTEM_VERSION}")

if(CMAKE_SYSTEM_VERSION VERSION_LESS 10.0.16299.0)
  message(FATAL_ERROR "Minimal Windows system version must be: 10.0.16299.0")
endif()

# Force package name to kodi, e.g. BUILD_KODI_ADDON not defined on addon depends build!
if(NOT PACKAGE_NAME)
  set(PACKAGE_NAME "kodi")
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
  cmake_policy(SET CMP0076 NEW)
endif()

option(ANGLE_ENABLE_NULL "Enable null renderer" OFF)
# windows options
option(ANGLE_ENABLE_D3D9 "Enable support D3D9 renderer (Windows desktop only)" OFF)
option(ANGLE_ENABLE_WGL "Enable support OpenGL renderer on Windows" OFF)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_definitions(-DANGLE_IS_32_BIT_CPU)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_definitions(-DANGLE_IS_64_BIT_CPU)
else()
  message(FATAL_ERROR "Unknown current CPU bit size: ${CMAKE_SIZEOF_VOID_P}")
endif()

if(WIN32 AND NOT WINDOWS_STORE)
    set(WINDOWS_DESKTOP TRUE)
else()
    set(WINDOWS_DESKTOP FALSE)
endif()
if (WINDOWS_DESKTOP OR WINDOWS_STORE)
    set(IS_WIN 1)
    set(WINDOWS_ANY TRUE)
else()
    set(WINDOWS_ANY FALSE)
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
else()
    set(LINUX FALSE)
endif()

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(IS_DEBUG TRUE)
else()
  set(IS_DEBUG FALSE)
endif()

if(WINDOWS_ANY)
  add_compile_options(/d2guard4 /Wv:18 /guard:cf)
else()
  add_compile_options(-std=c++17 -fPIC)
endif()

if(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit -framework CoreFoundation -framework IOSurface -framework QuartzCore -framework CoreGraphics")
endif()

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_DEBUG_POSTFIX "_dbg")

# if BUILD_SHARED_LIBS is undefined comes it to a cmake build error, set it as default to 0.
if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS 0)
endif()

add_definitions(
  -D_CRT_SECURE_NO_DEPRECATE
  -D_SCL_SECURE_NO_WARNINGS
  -D_HAS_EXCEPTIONS=0
  -DNOMINMAX
  -DANGLE_STANDALONE_BUILD)
if(IS_DEBUG)
  add_definitions(-DANGLE_ENABLE_DEBUG_ANNOTATIONS)
endif()
if(DCHECK_ALWAYS_ON)
  add_definitions(-DANGLE_ENABLE_RELEASE_ASSERTS)
endif()

# Shared library function visibility
if(IS_WIN)
  set(ANGLE_GL_VISIBILITY_CONFIG GL_APICALL=
                                 GL_API=)
else()
  set(ANGLE_GL_VISIBILITY_CONFIG GL_APICALL=__attribute__((visibility(\"default\")))
                                 GL_API=__attribute__((visibility(\"default\"))))
endif()

# This config controls export definitions on ANGLE API calls.
set(ANGLE_STATIC_DEFINES ANGLE_EXPORT=
                         ANGLE_UTIL_EXPORT=
                         EGLAPI=
                         GL_APICALL=
                         GL_API=)

set(GL_PROTOTYPES GL_GLES_PROTOTYPES=1
                  EGL_EGL_PROTOTYPES=1
                  GL_GLEXT_PROTOTYPES
                  EGL_EGLEXT_PROTOTYPES
                  HAS_ANGLE)

##########
# Generate required commit.h file
set(COMMIT_H
  "#define ANGLE_COMMIT_HASH \"invalid-hash\"\n"
  "#define ANGLE_COMMIT_HASH_SIZE 12\n"
  "#define ANGLE_COMMIT_DATE \"invalid-date\"\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/id/commit.h ${COMMIT_H})
include_directories(include ${CMAKE_CURRENT_SOURCE_DIR}/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/src
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/common/third_party/base
                            ${CMAKE_CURRENT_BINARY_DIR}/include)

##########
# Load angle source list by translating *.gni's to cmake format
set(GNI_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/compiler.gni
              ${CMAKE_CURRENT_SOURCE_DIR}/src/libGLESv2.gni
              ${CMAKE_CURRENT_SOURCE_DIR}/src/libANGLE/renderer/d3d/d3d11/d3d11_blit_shaders_autogen.gni
              ${CMAKE_CURRENT_SOURCE_DIR}/util/util.gni)

set(ANGLE_SOURCE_FILES "")
foreach(file ${GNI_FILES})
  get_filename_component(fullpath "${file}" ABSOLUTE)
  get_filename_component(file "${file}" NAME_WE)

  file(READ ${fullpath} CONTENT_GNI)
  string(REGEX REPLACE "([0-9a-z_]+)([^a-z]+)([*^\=)])" "set(\\1 XXXYYY \\1 ZZZ" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "is_win" "IS_WIN" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "is_linux" "IS_LINUX" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "is_android" "IS_ANDROID" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "is_fuchsia" "IS_FUCHSIA" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "is_mac" "IS_MAC" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "||" "OR" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "}" "endif()" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE " {" "" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "\"," "" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "\"" "" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE " [" "" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "]" ")" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE " ZZZ" "}" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "YYY " "{" CONTENT_GNI ${CONTENT_GNI})
  string(REPLACE "XXX" "$" CONTENT_GNI ${CONTENT_GNI})
  if(CMAKE_VERSION VERSION_LESS 3.13)
    string(REPLACE " src/" " \${CMAKE_CURRENT_SOURCE_DIR}/src/" CONTENT_GNI ${CONTENT_GNI})
    string(REPLACE " include/" " \${CMAKE_CURRENT_SOURCE_DIR}/include/" CONTENT_GNI ${CONTENT_GNI})
    string(REPLACE " util/" " \${CMAKE_CURRENT_SOURCE_DIR}/util/" CONTENT_GNI ${CONTENT_GNI})
  endif()
  file(WRITE ${CMAKE_BINARY_DIR}/angleSourceList-${file}.cmake ${CONTENT_GNI})
  list(APPEND ANGLE_SOURCE_FILES ${CMAKE_BINARY_DIR}/angleSourceList-${file}.cmake)
endforeach()

foreach(file ${ANGLE_SOURCE_FILES})
  include(${file})
endforeach()

##########
# angle::angle_system_utils
add_library(angle_system_utils OBJECT ${angle_system_utils_sources})
add_library(angle::system_utils ALIAS angle_system_utils)

##########
# angle::xxhash
add_library(angle_xxhash OBJECT ${xxhash_sources})
add_library(angle::xxhash ALIAS angle_xxhash)

##########
# angle::common
add_library(angle_common OBJECT ${libangle_common_sources}
                                ${libangle_includes}
                                ${CMAKE_SOURCE_DIR}/src/common/version.h)
add_library(angle::common ALIAS angle_common)

##########
# angle::image_util
add_library(angle_image_util OBJECT ${libangle_image_util_sources})
add_library(angle::image_util ALIAS angle_image_util)

##########
# angle::preprocessor
add_library(angle_preprocessor OBJECT ${angle_preprocessor_sources})
add_library(angle::preprocessor ALIAS angle_preprocessor)

##########
# angle::translator
add_library(angle_translator OBJECT ${angle_translator_sources}
                                    ${angle_translator_essl_sources}
                                    ${angle_translator_glsl_sources}
                                    ${angle_translator_hlsl_sources})
target_compile_definitions(angle_translator PUBLIC ANGLE_ENABLE_ESSL
                                                   ANGLE_ENABLE_GLSL
                                                   ANGLE_ENABLE_HLSL)
add_library(angle::translator ALIAS angle_translator)

##########
# libANGLE

## OpenGL Renderer
if((WINDOWS_DESKTOP AND ANGLE_ENABLE_WGL) OR LINUX OR APPLE)
  add_library(angle_renderer_opengl INTERFACE)
  if(WINDOWS_DESKTOP)
    target_sources(angle_renderer_opengl INTERFACE $<BUILD_INTERFACE:${libangle_gl_sources}>
                                                   $<BUILD_INTERFACE:${libangle_gl_wgl_sources}>)
  elseif(APPLE)
    target_sources(angle_renderer_opengl INTERFACE $<BUILD_INTERFACE:${libangle_gl_sources}>
                                                   $<BUILD_INTERFACE:${libangle_gl_cgl_sources}>)
  elseif(LINUX)
    target_sources(angle_renderer_opengl INTERFACE $<BUILD_INTERFACE:${libangle_gl_sources}>
                                                   $<BUILD_INTERFACE:${libangle_gl_glx_sources}>)
  endif()

  target_compile_definitions(angle_renderer_opengl INTERFACE ANGLE_ENABLE_OPENGL)
  add_library(angle::renderer::opengl ALIAS angle_renderer_opengl)
endif()

# D3D Renderers
if(WINDOWS_ANY)
  ## WinRT D3D Renderer
  if(WINDOWS_STORE)
    add_library(angle_renderer_winrt INTERFACE)
    target_sources(angle_renderer_winrt INTERFACE $<BUILD_INTERFACE:${libangle_d3d11_winrt_sources}>)
    target_link_libraries(angle_renderer_winrt INTERFACE d3dcompiler.lib)
    add_library(angle::renderer::winrt ALIAS angle_renderer_winrt)
  endif()

  ## Win32/d3d9 D3D Renderer
  if(WINDOWS_DESKTOP AND ANGLE_ENABLE_D3D9)
    add_library(angle_renderer_win32 INTERFACE)
    target_sources(angle_renderer_win32 INTERFACE $<BUILD_INTERFACE:${libangle_d3d9_sources}>
                                                  $<BUILD_INTERFACE:${libangle_d3d_shared_sources}>)
    target_compile_definitions(angle_renderer_win32 INTERFACE ANGLE_ENABLE_D3D9)
    target_link_libraries(angle_renderer_win32 INTERFACE d3d9.lib)
    add_library(angle::renderer::win32 ALIAS angle_renderer_win32)
    list(APPEND ANGLE_DEPEND_LIBRARIES d3d9.lib)
  endif()

  ## D3D11 Base renderer
  add_library(angle_renderer_d3d INTERFACE)
  target_sources(angle_renderer_d3d INTERFACE $<BUILD_INTERFACE:${libangle_d3d11_sources}>
                                              $<BUILD_INTERFACE:${libangle_d3d11_blit_shaders}>
                                              $<BUILD_INTERFACE:${libangle_d3d_shared_sources}>)
  if(WINDOWS_DESKTOP)
    target_sources(angle_renderer_d3d INTERFACE $<BUILD_INTERFACE:${libangle_d3d11_win32_sources}>)
  endif()
  target_compile_definitions(angle_renderer_d3d INTERFACE
    ANGLE_ENABLE_D3D11
    "ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES={ \"d3dcompiler_47.dll\", \"d3dcompiler_46.dll\", \"d3dcompiler_43.dll\" }"
  )
  target_link_libraries(angle_renderer_d3d INTERFACE d3d11.lib dxguid.lib)
  add_library(angle::renderer::d3d ALIAS angle_renderer_d3d)
  list(APPEND ANGLE_DEPEND_LIBRARIES d3d11.lib dxguid.lib)
endif()

## Core libANGLE library
if(LINUX OR APPLE)
  set(LIBANGLE_RENDERER_PLATFORM
    $<BUILD_INTERFACE:angle::renderer::opengl>
  )
elseif(WINDOWS_STORE)
  set(LIBANGLE_RENDERER_PLATFORM
    $<BUILD_INTERFACE:angle::renderer::d3d>
    $<BUILD_INTERFACE:angle::renderer::winrt>
  )
elseif(WINDOWS_DESKTOP)
  set(LIBANGLE_RENDERER_PLATFORM
    $<BUILD_INTERFACE:angle::renderer::d3d>
    $<$<BOOL:${ANGLE_ENABLE_D3D9}>:$<BUILD_INTERFACE:angle::renderer::win32>>
    $<$<BOOL:${ANGLE_ENABLE_WGL}>:$<BUILD_INTERFACE:angle::renderer::opengl>>
    gdi32
  )
else()
  set(LIBANGLE_RENDERER_PLATFORM )
endif()

if(ANGLE_ENABLE_NULL)
  add_library(angle_renderer_null INTERFACE)
  target_sources(angle_renderer_null INTERFACE $<BUILD_INTERFACE:${libangle_null_sources}>
                                               $<BUILD_INTERFACE:${libangle_gl_null_sources}>)
  add_library(angle::renderer::null ALIAS angle_renderer_null)
  list(APPEND LIBANGLE_RENDERER_PLATFORM $<BUILD_INTERFACE:angle::renderer::null>)
endif()

add_library(libANGLE INTERFACE)
target_sources(libANGLE INTERFACE $<BUILD_INTERFACE:${libangle_sources}>
                                  $<TARGET_OBJECTS:angle::common>
                                  $<TARGET_OBJECTS:angle::image_util>
                                  $<TARGET_OBJECTS:angle::system_utils>
                                  $<TARGET_OBJECTS:angle::translator>
                                  $<TARGET_OBJECTS:angle::preprocessor>
                                  $<TARGET_OBJECTS:angle::xxhash>)
target_link_libraries(libANGLE INTERFACE ${LIBANGLE_RENDERER_PLATFORM})
target_include_directories(libANGLE INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/khronos>)
target_compile_definitions(libANGLE INTERFACE LIBANGLE_IMPLEMENTATION
                                              $<$<BOOL:${ANGLE_ENABLE_NULL}>:ANGLE_ENABLE_NULL>
                                              $<$<BOOL:${LINUX}>:ANGLE_USE_X11>)
add_library(angle::libANGLE ALIAS libANGLE)

##########
# libGLESv2
add_library(libGLESv2 ${libglesv2_sources}
                      ${libangle_includes}
                      ${CMAKE_CURRENT_SOURCE_DIR}/src/libGLESv2/libGLESv2_autogen.def)
target_link_libraries(libGLESv2 PRIVATE $<BUILD_INTERFACE:angle::libANGLE>
                                        ${ANGLE_DEPEND_LIBRARIES})
if(BUILD_SHARED_LIBS)
  target_compile_definitions(libGLESv2 PRIVATE LIBGLESV2_IMPLEMENTATION
                                               ${GL_PROTOTYPES}
                                               ${ANGLE_GL_VISIBILITY_CONFIG})
else()
  target_compile_definitions(libGLESv2 PRIVATE LIBGLESV2_IMPLEMENTATION
                                               ${GL_PROTOTYPES}
                                       PUBLIC  ${ANGLE_STATIC_DEFINES})
endif()
target_include_directories(libGLESv2 PUBLIC $<INSTALL_INTERFACE:include>)
target_compile_definitions(libGLESv2 PUBLIC $<INSTALL_INTERFACE:${GL_PROTOTYPES}>)

##########
# libEGL
add_library(libEGL ${libegl_sources}
                   ${libangle_includes}
                   $<$<BOOL:${WINDOWS_ANY}>:${CMAKE_CURRENT_SOURCE_DIR}/src/libEGL/libEGL.def>)
target_link_libraries(libEGL PUBLIC libGLESv2)
target_include_directories(libEGL PUBLIC $<INSTALL_INTERFACE:include>)

##########
# Angle installation
set_target_properties(libGLESv2 PROPERTIES PREFIX "")
set_target_properties(libEGL PROPERTIES PREFIX "")

install(TARGETS libGLESv2
                libEGL
        EXPORT  ANGLEExport
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(EXPORT      ANGLEExport
        FILE        ${PACKAGE_NAME}-angle-config.cmake
        NAMESPACE   ${PACKAGE_NAME}::angle::
        DESTINATION share/${PACKAGE_NAME}-angle)

if(NOT DISABLE_INSTALL_HEADERS)
  install(DIRECTORY   include/
          DESTINATION include
          FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.inc"
            PATTERN "GLSLANG" EXCLUDE
            PATTERN "export.h" EXCLUDE)
endif()
