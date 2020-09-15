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

add_definitions(-DEASTL_STD_ITERATOR_CATEGORY_ENABLED)


#-------------------------------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------------------------------
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/../scripts/CMake")
include(CommonCppFlags)

#if (MSVC)
#	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
#endif()


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Remove warnings from EA libraries (used for tests and benchmarks only)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pointer-bool-conversion -Wno-unknown-warning-option -Wno-deprecated-declarations -Wno-ignored-pragma-optimize -Wno-microsoft-cast")
endif()

#-------------------------------------------------------------------------------------------
# Source files
#-------------------------------------------------------------------------------------------
file(GLOB EASTLTEST_SOURCES "source/*.cpp" "source/*.inl" "source/*.h")


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
#mb: EABase and EASTL are already found from 3rdparty
#add_subdirectory(packages/EABase)
add_subdirectory(packages/EAAssert)
add_subdirectory(packages/EAStdC)
#add_subdirectory(packages/EASTL)
add_subdirectory(packages/EAMain)
add_subdirectory(packages/EATest)
add_subdirectory(packages/EAThread)

set(EASTLTest_Libraries
    EABase
    EAAssert
    EAMain
    EASTL
    EAStdC
    EATest
    EAThread
    safe_memory
)

#target_link_libraries(EASTLTest EABase)
#target_link_libraries(EASTLTest EAAssert)
#target_link_libraries(EASTLTest EAMain)
#target_link_libraries(EASTLTest EASTL)
#target_link_libraries(EASTLTest EAStdC)
#target_link_libraries(EASTLTest EATest)
#target_link_libraries(EASTLTest EAThread)
#target_link_libraries(EASTLTest safe_memory)

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

