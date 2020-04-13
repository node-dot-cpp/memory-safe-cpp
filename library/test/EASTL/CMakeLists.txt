#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------
option(EASTL_BUILD_BENCHMARK "Enable generation of build files for benchmark" OFF)
option(EASTL_BUILD_TESTS "Enable generation of build files for tests" OFF)

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
if(EASTL_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

if(EASTL_BUILD_TESTS)
    add_subdirectory(test)
endif()
