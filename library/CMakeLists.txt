#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.0)
project(safememory CXX)

include(CTest)

set (CMAKE_MODULE_PATH 
  "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/src/iibmalloc/src/foundation/tools")
include(CommonCppFlags)

#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------


add_definitions(-D_CHAR16T)

add_definitions(-DUSING_T_SOCKETS)


#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
add_library(SafeMemory STATIC
  src/safe_ptr.cpp
  src/string.cpp
  src/hashtable.cpp
  )

#-------------------------------------------------------------------------------------------
# Export Include Directories
#-------------------------------------------------------------------------------------------
target_include_directories(SafeMemory
  PUBLIC include
  PUBLIC src
  )

#-------------------------------------------------------------------------------------------
# Package Dependencies 
#-------------------------------------------------------------------------------------------
add_subdirectory(src/iibmalloc)

target_link_libraries(SafeMemory iibmalloc)


#-------------------------------------------------------------------------------------------
# gcc_lto_workaround
#-------------------------------------------------------------------------------------------

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC

  add_library(gcc_lto_workaround SHARED
    gcc_lto_workaround/gcc_lto_workaround.cpp
    )

  target_link_libraries(gcc_lto_workaround foundation)
  target_link_libraries(SafeMemory gcc_lto_workaround)

endif()

#-------------------------------------------------------------------------------------------
# Tests 
#-------------------------------------------------------------------------------------------
add_executable(test_safe_memory
  test/test_safe_pointers.cpp
  )

if (MSVC AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    # emulating clang-cl specifics
    target_compile_options(test_safe_memory PRIVATE -Wno-missing-braces)
    target_compile_options(test_safe_memory PRIVATE -Wno-reinterpret-base-class)
endif()

target_link_libraries(test_safe_memory SafeMemory)

add_test(Run_test_safe_memory test_safe_memory)


#-------------------------------------------------------------------------------------------
# Subdirectories 
#-------------------------------------------------------------------------------------------
add_subdirectory(samples)
add_subdirectory(test/EASTL)
