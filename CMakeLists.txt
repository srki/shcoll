cmake_minimum_required(VERSION 3.8)
project(OpenSHMEM-Collective-Routines)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_COMPILER /Users/sm108/libs/SOS/_install/bin/oshcc)

include_directories(src/)

set(SOURCE_FILES
        src/util/trees.c
        src/barrier.c
        src/broadcast.c
        test/barrier_test.c
        test/broadcast_test.c)

add_executable(OpenSHMEM-Collective-Routines ${SOURCE_FILES})