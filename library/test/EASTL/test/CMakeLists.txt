#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# CMake info
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(EASTLTest CXX)
include(CTest)

#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-DEASTL_OPENSOURCE=1)
add_definitions(-D_CHAR16T)
add_definitions(-DEASTL_THREAD_SUPPORT_AVAILABLE=0)

#-------------------------------------------------------------------------------------------
# Source files
#-------------------------------------------------------------------------------------------
file(GLOB EASTLTEST_SOURCES "source/*.cpp" "source/*.inl" "source/*.h")
set(SOURCES ${EASTLTEST_SOURCES})

#-------------------------------------------------------------------------------------------
# Executable definition
#-------------------------------------------------------------------------------------------
add_executable(EASTLTest ${EASTLTEST_SOURCES})
add_executable(EASTLTestNoCheck ${EASTLTEST_SOURCES})

target_compile_definitions(EASTLTestNoCheck PUBLIC NODECPP_MEMORY_SAFETY=0)

#-------------------------------------------------------------------------------------------
# Include directories
#-------------------------------------------------------------------------------------------
target_include_directories(EASTLTest PUBLIC include)
target_include_directories(EASTLTestNoCheck PUBLIC include)


#-------------------------------------------------------------------------------------------
# Dependencies 
#-------------------------------------------------------------------------------------------
# add_subdirectory(packages/EABase)
add_subdirectory(packages/EAAssert)
add_subdirectory(packages/EAStdC)
add_subdirectory(packages/EAMain)
add_subdirectory(packages/EATest)
add_subdirectory(packages/EAThread)

set(EASTLTest_Libraries
    EABase
    EAAssert
    EAMain
    EAThread
    EAStdC
    EASTL
    EATest
    safememory)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if((NOT APPLE) AND (NOT WIN32))
    target_link_libraries(EASTLTest ${EASTLTest_Libraries} Threads::Threads rt)
    target_link_libraries(EASTLTestNoCheck ${EASTLTest_Libraries} Threads::Threads rt)
else()
    target_link_libraries(EASTLTest ${EASTLTest_Libraries} Threads::Threads)
    target_link_libraries(EASTLTestNoCheck ${EASTLTest_Libraries} Threads::Threads)
endif()

#-------------------------------------------------------------------------------------------
# Run Unit tests and verify the results.
#-------------------------------------------------------------------------------------------
add_test(EASTLTestRuns EASTLTest)
add_test(EASTLTestNoCheckRuns EASTLTestNoCheck)

set_tests_properties (EASTLTestRuns PROPERTIES PASS_REGULAR_EXPRESSION "RETURNCODE=0")
set_tests_properties (EASTLTestNoCheckRuns PROPERTIES PASS_REGULAR_EXPRESSION "RETURNCODE=0")
