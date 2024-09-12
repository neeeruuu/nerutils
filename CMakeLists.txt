cmake_minimum_required(VERSION 3.92.2)
set(CMAKE_CXX_STANDARD 20)

project(nerutils LANGUAGES CXX)

file(GLOB_RECURSE NERUTILS_SOURCES CONFIGURE_DEPENDS "src/*.h" "src/*.cpp")
add_library(nerutils STATIC ${NERUTILS_SOURCES})
target_include_directories(nerutils PUBLIC src)