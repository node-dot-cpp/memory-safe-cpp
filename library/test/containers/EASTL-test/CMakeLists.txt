#-------------------------------------------------------------------------------------------
# Copyright (c) 2021, OLogN Technologies AG
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# CMake info
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(SafeMemoryTests CXX)
include(CTest)

#-------------------------------------------------------------------------------------------
# Source files
#-------------------------------------------------------------------------------------------
set(SafeMemoryTests_Sources
    EASTLTest.cpp
    main.cpp
    TestArray.cpp
    TestHash.cpp
    TestString.cpp
    TestVector.cpp
)

#-------------------------------------------------------------------------------------------
# Executable definition
#-------------------------------------------------------------------------------------------
# Safety use cases
if (MSVC)
    # sometimes cl.exe will issue an error asking this, other times just hangs
    add_compile_options(/bigobj)
endif ()


add_executable(SafeMemoryTests ${SafeMemoryTests_Sources})
target_link_libraries(SafeMemoryTests safememory)

add_executable(SafeMemoryTestsImpl ${SafeMemoryTests_Sources})
target_link_libraries(SafeMemoryTestsImpl safememory_impl)

add_executable(SafeMemoryTestsNoChecks ${SafeMemoryTests_Sources})
target_link_libraries(SafeMemoryTestsNoChecks safememory_no_checks)

add_executable(SafeMemoryTestsOnDemandStd ${SafeMemoryTests_Sources})
target_link_libraries(SafeMemoryTestsOnDemandStd safememory)
target_compile_definitions(SafeMemoryTestsOnDemandStd PUBLIC SAFEMEMORY_TEST_DONT_INITIALIZE_IIBALLOC)

add_executable(SafeMemoryTestsDzIt ${SafeMemoryTests_Sources})
target_link_libraries(SafeMemoryTestsDzIt safememory_dz_it)

#-------------------------------------------------------------------------------------------
# Run Unit tests and verify the results.
#-------------------------------------------------------------------------------------------
add_test(SafeMemoryTestsRun SafeMemoryTests)
add_test(SafeMemoryTestsImplRun SafeMemoryTestsImpl)
add_test(SafeMemoryTestsNoChecksRun SafeMemoryTestsNoChecks)
add_test(SafeMemoryTestsOnDemandStdRun SafeMemoryTestsOnDemandStd)
add_test(SafeMemoryTestsDzItRun SafeMemoryTestsDzIt)
