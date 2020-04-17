#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
add_library(SafeMemory STATIC
  src/safe_ptr.cpp
  )

#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
#add_definitions(-DEASTL_OPENSOURCE=1)
add_definitions(-D_CHAR16T)

add_definitions(-DUSING_T_SOCKETS)

if(MSVC)
  add_compile_options( /wd4996 )
  add_compile_options( /wd4267 )
  add_compile_options( /wd4535 )
	add_compile_options(/W3)
	add_compile_options(/EHa)
#	add_compile_options(/MD)
else()
	add_compile_options( -Wall )
	add_compile_options( -Wextra )
	add_compile_options( -Wno-unused-variable )
	add_compile_options( -Wno-unused-parameter )
	add_compile_options( -Wno-empty-body )
#	add_compile_options(-lpthread)
  add_compile_options( -O2 )
endif()

#-------------------------------------------------------------------------------------------
# Export Include Directories
#-------------------------------------------------------------------------------------------
target_include_directories(SafeMemory
  PUBLIC experimental/containers/include
  PUBLIC include
  PUBLIC src
  )

#-------------------------------------------------------------------------------------------
# Package Dependencies 
#-------------------------------------------------------------------------------------------
add_subdirectory(src/iibmalloc)

target_link_libraries(SafeMemory iibmalloc)


#-------------------------------------------------------------------------------------------
# Tests 
#-------------------------------------------------------------------------------------------

add_executable(test_safe_memory
  test/test_safe_pointers.cpp
  )

target_link_libraries(test_safe_memory SafeMemory)

add_test(Run_test_safe_memory test_safe_memory)


#-------------------------------------------------------------------------------------------
# gcc_lto_workaround
#-------------------------------------------------------------------------------------------

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC

  add_library(gcc_lto_workaround SHARED
    gcc_lto_workaround.cpp
    )

  add_compile_options(-fpic)

  target_link_libraries(gcc_lto_workaround foundation)
  
  add_executable(test_safe_memory_lto
    test/test_safe_pointers.cpp
    gcc_lto_workaround/gcc_lto_workaround.cpp
    )

  target_link_libraries(test_safe_memory_lto SafeMemory)

  add_test(Run_test_safe_memory_lto test_safe_memory_lto)

  add_executable(test_safe_memory_lto_wa
    test/test_safe_pointers.cpp
    )

  target_link_libraries(test_safe_memory_lto_wa SafeMemory gcc_lto_workaround)

  add_test(Run_test_safe_memory_lto_wa test_safe_memory_lto_wa)

endif()


