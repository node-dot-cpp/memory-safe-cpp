
add_compile_options(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
add_compile_options(-D_CRT_NONSTDC_NO_DEPRECATE)
add_compile_options(-Wno-unused-variable)



add_executable(container-test
  main.cpp
  ${PROJECT_SOURCE_DIR}/../../../library/src/safe_ptr.cpp
	${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/page_allocator_windows.cpp
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/iibmalloc_windows.cpp
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/foundation/3rdparty/fmt/src/format.cc
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/foundation/src/log.cpp
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/foundation/src/std_error.cpp
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/foundation/src/tagged_ptr_impl.cpp
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/foundation/src/safe_memory_error.cpp
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/foundation/src/cpu_exceptions_translator.cpp
  )

include_directories(
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/foundation/3rdparty/fmt/include
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src/foundation/include
  ${PROJECT_SOURCE_DIR}/../../../library/src/iibmalloc/src
  ${PROJECT_SOURCE_DIR}/../../../library/test
  ${PROJECT_SOURCE_DIR}/../../../library/src	)



set_property(TARGET container-test PROPERTY CXX_STANDARD 17)


