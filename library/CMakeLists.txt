#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12.4)
project(SafeMemory CXX)
set(CMAKE_CXX_STANDARD 20)
include(CTest)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/src/iibmalloc/src/foundation/tools")
include(CommonCppFlags)

add_subdirectory(3rdparty/EABase)
add_subdirectory(3rdparty/EASTL)

target_compile_definitions(EASTL PUBLIC EASTL_EXCEPTIONS_ENABLED)
target_compile_definitions(EASTL PUBLIC EASTL_STRING_OPT_RANGE_ERRORS=0)
target_compile_definitions(EASTL PUBLIC EASTL_STD_ITERATOR_CATEGORY_ENABLED)
target_compile_definitions(EASTL PUBLIC EASTL_EASTDC_VSNPRINTF=0)
target_compile_definitions(EASTL PUBLIC EASTL_SIZE_T_32BIT)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(EASTL PUBLIC -Wno-unused-but-set-variable )
  target_compile_options(EASTL PUBLIC -Wno-array-bounds )
  target_compile_options(EASTL PUBLIC -Wno-strict-aliasing )
endif()


add_subdirectory(src/iibmalloc)

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
SET(safememory_SRC
    "src/safe_ptr.cpp" 
    "src/string.cpp" 
    "src/nodecpp_error.cpp" 
    "src/detail/allocator_to_eastl.cpp"
)


add_library(safememory STATIC ${safememory_SRC})

# target_compile_definitions(safememory PUBLIC NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION)
# target_compile_definitions(safememory PUBLIC NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO)
# target_compile_definitions(safememory PUBLIC NODECPP_SAFEMEMORY_HEAVY_DEBUG)
# target_compile_definitions(safememory PUBLIC NODECPP_NOT_USING_IIBMALLOC)
# target_compile_definitions(foundation PUBLIC NODECPP_SAFE_PTR_DEBUG_MODE)
# target_compile_definitions(safememory PUBLIC NODECPP_MEMORY_SAFETY=0)
# target_compile_definitions(safememory PUBLIC SAFEMEMORY_DEZOMBIEFY_ITERATORS)


target_include_directories(safememory PUBLIC include)

target_link_libraries(safememory iibmalloc)
target_link_libraries(safememory EASTL EABase)

#-------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------
# Tests and samples
#-------------------------------------------------------------------------------------------
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(SAFEMEMORY_TEST ON CACHE BOOL "Enable SafeMemory tests")
else()
  set(SAFEMEMORY_TEST OFF CACHE BOOL "Enable SafeMemory tests")
endif()

if (SAFEMEMORY_TEST)

#-------------------------------------------------------------------------------------------
  add_library(safememory_impl STATIC ${safememory_SRC})
  target_compile_definitions(safememory_impl PUBLIC NODECPP_MEMORY_SAFETY=1)
  target_include_directories(safememory_impl PUBLIC include)
  target_link_libraries(safememory_impl iibmalloc EASTL EABase)

#-------------------------------------------------------------------------------------------
  add_library(safememory_no_checks STATIC ${safememory_SRC})
  target_compile_definitions(safememory_no_checks PUBLIC NODECPP_MEMORY_SAFETY=-1)
  target_include_directories(safememory_no_checks PUBLIC include)
  target_link_libraries(safememory_no_checks iibmalloc EASTL EABase)

#-------------------------------------------------------------------------------------------
  add_library(safememory_dz_it STATIC ${safememory_SRC})
  target_compile_definitions(safememory_dz_it PUBLIC SAFEMEMORY_DEZOMBIEFY_ITERATORS)
  target_include_directories(safememory_dz_it PUBLIC include)
  target_link_libraries(safememory_dz_it iibmalloc EASTL EABase)

endif()
#-------------------------------------------------------------------------------------------
# gcc_lto_workaround
#-------------------------------------------------------------------------------------------

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC

  add_library(gcc_lto_workaround SHARED
    gcc_lto_workaround/gcc_lto_workaround.cpp
    )

  target_link_libraries(safememory gcc_lto_workaround)

  if (SAFEMEMORY_TEST)
    target_link_libraries(safememory_impl gcc_lto_workaround)
    target_link_libraries(safememory_no_checks gcc_lto_workaround)
    target_link_libraries(safememory_dz_it gcc_lto_workaround)
  endif()
endif()

#-------------------------------------------------------------------------------------------
# Tests and samples
#-------------------------------------------------------------------------------------------

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

  add_subdirectory(test/containers/EASTL-benchmark)
  add_subdirectory(test/containers/EASTL-test)
  add_subdirectory(test/containers/dezombiefy)
  add_subdirectory(test/containers/zeroed)


endif()
