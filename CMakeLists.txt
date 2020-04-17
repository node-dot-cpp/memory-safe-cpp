#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.0)
project(safememory CXX)
set(CMAKE_CXX_STANDARD 17)
include(CTest)


#-------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------
option(SAFEMEMORY_BUILD_CHECKER "Enable generation of checker tool" OFF)

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
add_subdirectory(library)

if(SAFEMEMORY_BUILD_CHECKER)
    add_subdirectory(checker)
endif()


