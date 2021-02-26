#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------
option(SAFEMEMORY_BUILD_BENCHMARK "Enable generation of build files for benchmark" OFF)
option(SAFEMEMORY_BUILD_TESTS "Enable generation of build files for tests" OFF)

#-------------------------------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------------------------------
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/scripts/CMake")
include(CommonCppFlags)

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
if(SAFEMEMORY_BUILD_BENCHMARK)
   add_subdirectory(benchmark)
endif()

if(SAFEMEMORY_BUILD_TESTS)
    add_subdirectory(test)
endif()
