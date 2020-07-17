#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.0)
project(SafeMemory CXX)
set(CMAKE_CXX_STANDARD 17)
include(CTest)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/src/iibmalloc/src/foundation/tools")
include(CommonCppFlags)


#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
add_library(safe_memory STATIC
  src/EASTL/string.cpp
  src/EASTL/hashtable.cpp
  src/safe_ptr.cpp
  src/nodecpp_error.cpp
  )

target_include_directories(safe_memory
  PUBLIC include
  PUBLIC src
  )

add_subdirectory(src/iibmalloc)

target_link_libraries(safe_memory iibmalloc)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # remove default exceptions, because of warning
  string( REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
endif()

#-------------------------------------------------------------------------------------------
# gcc_lto_workaround
#-------------------------------------------------------------------------------------------

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC

  add_library(gcc_lto_workaround SHARED
    gcc_lto_workaround/gcc_lto_workaround.cpp
    )

  target_link_libraries(gcc_lto_workaround foundation)
  target_link_libraries(safe_memory gcc_lto_workaround)

endif()

#-------------------------------------------------------------------------------------------
# Tests and samples
#-------------------------------------------------------------------------------------------
add_executable(test_safe_memory
  test/test_safe_pointers.cpp
  )

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(test_safe_memory PRIVATE -Wno-missing-braces)
    target_compile_options(test_safe_memory PRIVATE -Wno-reinterpret-base-class)
    target_compile_options(test_safe_memory PRIVATE -Wno-deprecated-declarations)
endif()

target_link_libraries(test_safe_memory safe_memory)

add_test(Run_test_safe_memory test_safe_memory)

add_subdirectory(samples)
add_subdirectory(test/EASTL)
