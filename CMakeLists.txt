cmake_minimum_required(VERSION 3.21)

set(SOBB_VERSION "0.0.1")
project(sobb VERSION ${SOBB_VERSION} LANGUAGES C CXX DESCRIPTION "graphics shenanigans")

find_package(Threads REQUIRED)

add_subdirectory(extern)

add_subdirectory(support)
add_subdirectory(src)
