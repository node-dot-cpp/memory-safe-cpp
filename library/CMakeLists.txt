#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.0)
project(SafeMemory CXX)
set(CMAKE_CXX_STANDARD 17)
include(CTest)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/src/iibmalloc/src/foundation/tools")
include(CommonCppFlags)

# TODO remove conditional use of EASTL as it now is embedded
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/EASTL)
  add_subdirectory(3rdparty/EABase)
  add_subdirectory(3rdparty/EASTL)
  target_compile_definitions(EASTL PUBLIC EASTL_EXCEPTIONS_ENABLED)
endif()
add_subdirectory(src/iibmalloc)


#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
add_library(safememory STATIC
  src/safe_ptr.cpp
  src/nodecpp_error.cpp
  src/detail/allocator_to_eastl.cpp
  )

target_include_directories(safememory
  PUBLIC include
  PUBLIC src
  )

target_link_libraries(safememory iibmalloc)
if (TARGET EASTL)
  target_link_libraries(safememory EASTL EABase)
endif()

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
  target_link_libraries(safememory gcc_lto_workaround)

endif()

#-------------------------------------------------------------------------------------------
# Tests and samples
#-------------------------------------------------------------------------------------------
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(SAFEMEMORY_TEST ON CACHE BOOL "Enable SafeMemory tests")
else()
  set(SAFEMEMORY_TEST OFF CACHE BOOL "Enable SafeMemory tests")
endif()

if (SAFEMEMORY_TEST)
  # only when not a subproject

  add_executable(test_safememory
    test/test_safe_pointers.cpp
    )

  target_compile_definitions(test_safememory PRIVATE NODECPP_MEMORY_SAFETY_EXCLUSIONS="${CMAKE_CURRENT_SOURCE_DIR}/test/safety_exclusions.h")

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(test_safememory PRIVATE -Wno-missing-braces)
      target_compile_options(test_safememory PRIVATE -Wno-reinterpret-base-class)
      target_compile_options(test_safememory PRIVATE -Wno-deprecated-declarations)
  endif()

  target_link_libraries(test_safememory safememory)

  add_test(Run_test_safememory test_safememory)

  add_subdirectory(samples)

# When EASTL is present add container tests 
  if (TARGET EASTL)
    add_subdirectory(test/EASTL)
  endif()
endif()
