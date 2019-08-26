cmake_minimum_required(VERSION 3.5)
project(kissfft)

set(SOURCES kiss_fft.c)

add_library(kissfft STATIC ${SOURCES})
set_property(TARGET kissfft PROPERTY POSITION_INDEPENDENT_CODE ON)
