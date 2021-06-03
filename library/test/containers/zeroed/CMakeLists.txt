#-------------------------------------------------------------------------------------------
# Copyright (c) 2021, OLogN Technologies AG
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# Executable definition
#-------------------------------------------------------------------------------------------

add_executable(test_zeroed_containers test_zeroed_containers.cpp)
target_link_libraries(test_zeroed_containers safememory)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(test_zeroed_containers PRIVATE -Wno-class-memaccess)
endif()

#-------------------------------------------------------------------------------------------
# Run Unit tests and verify the results.
#-------------------------------------------------------------------------------------------
add_test(test_zeroed_containersRun test_zeroed_containers)
