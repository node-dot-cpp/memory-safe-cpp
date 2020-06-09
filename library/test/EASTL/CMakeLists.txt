#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------
option(SAFEMEMORY_BUILD_BENCHMARK "Enable generation of build files for benchmark" ON)
option(SAFEMEMORY_BUILD_TESTS "Enable generation of build files for tests" ON)

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
if(SAFEMEMORY_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

if(SAFEMEMORY_BUILD_TESTS)
    add_subdirectory(test)
endif()
