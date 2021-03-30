#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# CMake info
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(SafeMemoryTests CXX)
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
file(GLOB EASTLTEST_SOURCES "*.cpp" "*.inl" "*.h")
set(SOURCES ${EASTLTEST_SOURCES})

#-------------------------------------------------------------------------------------------
# Executable definition
#-------------------------------------------------------------------------------------------
# Safety use cases
add_executable(SafeMemoryTests ${EASTLTEST_SOURCES})

add_executable(SafeMemoryTestsImpl ${EASTLTEST_SOURCES})
add_executable(SafeMemoryTestsNoChecks ${EASTLTEST_SOURCES})
add_executable(SafeMemoryTestsOnDemandStd ${EASTLTEST_SOURCES})

#-------------------------------------------------------------------------------------------
# Include directories
#-------------------------------------------------------------------------------------------
target_include_directories(SafeMemoryTests PUBLIC include)

target_include_directories(SafeMemoryTestsImpl PUBLIC include)
target_include_directories(SafeMemoryTestsNoChecks PUBLIC include)
target_include_directories(SafeMemoryTestsOnDemandStd PUBLIC include)

target_compile_definitions(SafeMemoryTestsOnDemandStd PUBLIC SAFEMEMORY_TEST_DONT_INITIALIZE_IIBALLOC)

#-------------------------------------------------------------------------------------------
# Dependencies 
#-------------------------------------------------------------------------------------------

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (MSVC)
  target_compile_options(SafeMemoryTests PRIVATE /bigobj)
  target_compile_options(SafeMemoryTestsImpl PRIVATE /bigobj)
  target_compile_options(SafeMemoryTestsNoChecks PRIVATE /bigobj)
  target_compile_options(SafeMemoryTestsOnDemandStd PRIVATE /bigobj)
endif ()


target_link_libraries(SafeMemoryTests safememory Threads::Threads)
target_link_libraries(SafeMemoryTestsImpl safememory_impl Threads::Threads)
target_link_libraries(SafeMemoryTestsNoChecks safememory_no_checks Threads::Threads)
target_link_libraries(SafeMemoryTestsOnDemandStd safememory Threads::Threads)

if((NOT APPLE) AND (NOT WIN32))
    target_link_libraries(SafeMemoryTests rt)
    target_link_libraries(SafeMemoryTestsImpl rt)
    target_link_libraries(SafeMemoryTestsNoChecks rt)
    target_link_libraries(SafeMemoryTestsOnDemandStd rt)
endif()

#-------------------------------------------------------------------------------------------
# Run Unit tests and verify the results.
#-------------------------------------------------------------------------------------------
add_test(SafeMemoryTestsRun SafeMemoryTests)
add_test(SafeMemoryTestsImplRun SafeMemoryTestsImpl)
add_test(SafeMemoryTestsNoChecksRun SafeMemoryTestsNoChecks)
add_test(SafeMemoryTestsOnDemandStdRun SafeMemoryTestsOnDemandStd)
