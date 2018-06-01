cmake_minimum_required(VERSION 3.8)
project(OpenSHMEM-Collective-Routines)

#set(CMAKE_C_STANDARD 99)

set(CMAKE_C_COMPILER /Users/sm108/libs/SOS/_install/bin/oshcc)

include_directories(src/)

set(SOURCE_FILES
        src/util/trees.c
        src/util/rotate.c
        src/util/scan.c
        src/util/wraper_1_3.h
        src/barrier.c
        src/broadcast.c
        src/reduction.c
        src/collect.c
        src/fcollect.c
        src/alltoall.c
        src/alltoalls.c

        test/barrier_test.c
        test/broadcast_test.c
        test/reduction_test.c
        test/collect_test.c
        test/fcollect_test.c
        test/alltoall_test.c
        test/alltoalls_test.c
        test/util/debug.h
        test/util/util.h
        test/util/run.h
        src/util/bithacks.c src/util/bithacks.h src/reduction_template.h.hpp)


add_executable(OpenSHMEM-Collective-Routines ${SOURCE_FILES})