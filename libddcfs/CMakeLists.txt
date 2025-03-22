cmake_minimum_required(VERSION 3.13)
project(
  libddcfs
  VERSION 1.0.0
  LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wformat -Wunused-variable -Werror=format -Werror=return-type"
)

include_directories(include)
add_library(ddcfs src/ddcfs.cc src/overrride.cc)