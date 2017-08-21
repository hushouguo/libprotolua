set(target protolua) 
project (${target})
cmake_minimum_required (VERSION 2.8)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_BUILD_TYPE "Debug")#Release
set(EXECUTABLE_OUTPUT_PATH .)
add_definitions(-Wall -Werror -D_GNU_SOURCE -DLINUX -DX64 -DDEBUG -std=c++11)
include_directories(src/)
include_directories(deps/luajit-2.0.5/include/)
include_directories(deps/protobuf-3.3.0/include/)
aux_source_directory(src src)
add_library(${target} ${src})

