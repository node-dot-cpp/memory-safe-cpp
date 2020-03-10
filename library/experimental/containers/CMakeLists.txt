
add_compile_options( /wd4996 )
add_compile_options( /wd4267 )
add_compile_options( /wd4535 )

add_executable(container-test
  main.cpp
  source/hashtable.cpp
  ${PROJECT_SOURCE_DIR}/../../src/safe_ptr.cpp
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/iibmalloc.cpp
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/3rdparty/fmt/src/format.cc
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/src/nodecpp_assert.cpp
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/src/page_allocator.cpp
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/src/log.cpp
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/src/std_error.cpp
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/src/tagged_ptr_impl.cpp
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/src/safe_memory_error.cpp
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/src/cpu_exceptions_translator.cpp
  )

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/3rdparty/fmt/include
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src/foundation/include
  ${PROJECT_SOURCE_DIR}/../../src/iibmalloc/src
  ${PROJECT_SOURCE_DIR}/../../test
  ${PROJECT_SOURCE_DIR}/../../src	)




set_property(TARGET container-test PROPERTY CXX_STANDARD 17)


