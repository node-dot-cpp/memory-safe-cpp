#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# CMake info
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(SafeMemoryBenchmarks CXX)
include(CTest)

#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------
add_definitions(-D_CHAR16T)

#-------------------------------------------------------------------------------------------
# Include directories
#-------------------------------------------------------------------------------------------
#include_directories(source)
#include_directories(../test/source)

#-------------------------------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------------------------------
#set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/../scripts/CMake")
#include(CommonCppFlags)


#-------------------------------------------------------------------------------------------
# Source files
#-------------------------------------------------------------------------------------------
set(EASTLBenchmark_Sources
    BenchmarkHash.cpp
    BenchmarkString.cpp
    BenchmarkVector.cpp
    EASTLBenchMark.cpp
    EASTLTest.cpp
    EAStopwatch.cpp
    main.cpp
)


#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-DEASTL_THREAD_SUPPORT_AVAILABLE=0)
add_definitions(-DEASTL_OPENSOURCE=1)
add_definitions(-D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)  # silence std::hash_map deprecation warnings

#if(NOT SAFEMEMORY_BUILD_TESTS)
#    add_subdirectory(../test/packages/EAStdC ../test/EAStdC)
#    add_subdirectory(../test/packages/EAAssert ../test/EAAssert)
#    add_subdirectory(../test/packages/EAThread ../test/EAThread)
#    add_subdirectory(../test/packages/EATest ../test/EATest)
#    add_subdirectory(../test/packages/EAMain ../test/EAMain)
#endif()

#-------------------------------------------------------------------------------------------
# Executable definition
#-------------------------------------------------------------------------------------------
add_executable(SafeMemoryBenchmarks ${EASTLBenchmark_Sources})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(EASTLBenchmark_Libraries
    EABase
#    EAAssert
#    EAMain
#    EAThread
#    EAStdC
    EASTL
#    EATest
    safememory)
target_link_libraries(SafeMemoryBenchmarks ${EASTLBenchmark_Libraries} Threads::Threads)

#-------------------------------------------------------------------------------------------
# Run Unit tests and verify the results.
#-------------------------------------------------------------------------------------------
add_test(SafeMemoryBenchmarkRuns SafeMemoryBenchmarks)
set_tests_properties (SafeMemoryBenchmarkRuns PROPERTIES PASS_REGULAR_EXPRESSION "RETURNCODE=0")

