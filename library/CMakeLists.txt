#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(SafeMemory CXX)

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
add_library(SafeMemory
  src/safe_ptr.cpp
  experimental/containers/source/hashtable.cpp
  src/iibmalloc/src/iibmalloc.cpp
  src/iibmalloc/src/foundation/3rdparty/fmt/src/format.cc
  src/iibmalloc/src/foundation/src/nodecpp_assert.cpp
  src/iibmalloc/src/foundation/src/page_allocator.cpp
  src/iibmalloc/src/foundation/src/log.cpp
  src/iibmalloc/src/foundation/src/std_error.cpp
  src/iibmalloc/src/foundation/src/tagged_ptr_impl.cpp
  src/iibmalloc/src/foundation/src/safe_memory_error.cpp
  src/iibmalloc/src/foundation/src/cpu_exceptions_translator.cpp
  )

set_target_properties(SafeMemory PROPERTIES CXX_STANDARD 17)

#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------
add_compile_options( /wd4996 )
add_compile_options( /wd4267 )
add_compile_options( /wd4535 )
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-DEASTL_OPENSOURCE=1)
add_definitions(-D_CHAR16T)
add_definitions(-DEASTL_THREAD_SUPPORT_AVAILABLE=0)


#-------------------------------------------------------------------------------------------
# Export Include Directories
#-------------------------------------------------------------------------------------------
target_include_directories(SafeMemory
  PUBLIC experimental/containers/include
  PUBLIC src/iibmalloc/src/foundation/3rdparty/fmt/include
  PUBLIC src/iibmalloc/src/foundation/include
  PUBLIC src/iibmalloc/src
  PUBLIC src
  )


#-------------------------------------------------------------------------------------------
# Package Dependencies 
#-------------------------------------------------------------------------------------------
#target_link_libraries(SafeMemory Iibmalloc Foundation)

add_subdirectory(test/EASTL)
add_subdirectory(experimental/containers)

