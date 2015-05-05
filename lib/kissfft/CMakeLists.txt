project(kissfft)

enable_language(C)

cmake_minimum_required(VERSION 2.6)

set(SOURCES kiss_fft.c)

if(NOT WIN32)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

add_library(kissfft STATIC ${SOURCES})
