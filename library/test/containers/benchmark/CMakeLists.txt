#-------------------------------------------------------------------------------------------
# Copyright (c) 2021, OLogN Technologies AG
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# CMake info
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(SafeMemoryBenchmarks CXX)
include(CTest)


#-------------------------------------------------------------------------------------------
# Executable definition
#-------------------------------------------------------------------------------------------
add_executable(SafeMemoryBenchmarks
    BenchmarkHash.cpp
    BenchmarkString.cpp
    BenchmarkVector.cpp
    EASTLBenchmark.cpp
    EASTLTest.cpp
    EAStopwatch.cpp
    main.cpp
)

target_link_libraries(SafeMemoryBenchmarks safememory)

#-------------------------------------------------------------------------------------------
# Run Unit tests and verify the results.
#-------------------------------------------------------------------------------------------
add_test(SafeMemoryBenchmarksRun SafeMemoryBenchmarks)

