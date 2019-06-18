cmake_minimum_required(VERSION 3.0)
project(iibmalloc)

find_package(Threads)

set(TEST_BIN_NAME test_safe_memory.bin)
set(TEST_BIN_NAME_WITH_LTO test_safe_memory_lto.bin)
set(TEST_BIN_NAME_WITH_LTO_WORKAROUND test_safe_memory_lto_wa.bin)

set(LIB_NAME gcc_lto_workaround)

add_subdirectory(${PROJECT_SOURCE_DIR}/library/src/iibmalloc build)

if(MSVC OR MSYS OR MINGW)
	option(CMAKE_USE_WIN32_THREADS_INIT "using WIN32 threads" ON)
	add_definitions(/W3)
	add_definitions(/EHa)
	add_definitions(/MD)
else()
	enable_testing()
	add_definitions( -g )
	add_definitions( -Wall )
	add_definitions( -Wextra )
	add_definitions( -Wno-unused-variable )
	add_definitions( -Wno-unused-parameter )
	add_definitions( -Wno-empty-body )
	add_definitions(-lpthread)
	add_compile_options(-O2)
	set(CMAKE_CXX_STANDARD_REQUIRED on)
	set(CMAKE_CXX_FLAGS_DEBUG " ${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DUSING_T_SOCKETS ")
	set(CMAKE_CXX_FLAGS_RELEASE " ${CMAKE_CXX_FLAGS_RELEASE}  -DNDEBUG -DUSING_T_SOCKETS ")
endif()


include_directories(include  ${PROJECT_SOURCE_DIR}/library/src/iibmalloc/src/foundation/3rdparty/fmt/include
							 ${PROJECT_SOURCE_DIR}/library/src/iibmalloc/src/foundation/include
							 ${PROJECT_SOURCE_DIR}/library/src/iibmalloc/src
							 ${PROJECT_SOURCE_DIR}/library/test
							 ${PROJECT_SOURCE_DIR}/library/src	)


set(SOURCE_EXE ${PROJECT_SOURCE_DIR}/library/test/test_safe_pointers.cpp 
			   ${PROJECT_SOURCE_DIR}/library/src/safe_ptr.cpp)

set(SOURCE_LIB ${PROJECT_SOURCE_DIR}/library/gcc_lto_workaround/gcc_lto_workaround.cpp)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)

add_executable(${TEST_BIN_NAME} ${SOURCE_EXE})
add_executable(${TEST_BIN_NAME_WITH_LTO}  ${SOURCE_EXE} ${PROJECT_SOURCE_DIR}/library/gcc_lto_workaround/gcc_lto_workaround.cpp)
add_executable(${TEST_BIN_NAME_WITH_LTO_WORKAROUND} ${SOURCE_EXE})

add_library( ${LIB_NAME} SHARED ${SOURCE_LIB})

target_link_libraries( ${TEST_BIN_NAME} iibmalloc )
target_link_libraries( ${TEST_BIN_NAME_WITH_LTO} iibmalloc )
target_link_libraries( ${TEST_BIN_NAME_WITH_LTO_WORKAROUND} iibmalloc ${LIB_NAME} )

if (UNIX)

	target_link_libraries( ${TEST_BIN_NAME}  pthread)
	target_link_libraries( ${TEST_BIN_NAME_WITH_LTO}  pthread)
	target_link_libraries( ${TEST_BIN_NAME_WITH_LTO_WORKAROUND}  ${LIB_NAME} pthread)

	set_target_properties ( ${LIB_NAME} PROPERTIES COMPILE_FLAGS  "-fpic ")
	set_target_properties ( ${TEST_BIN_NAME} PROPERTIES COMPILE_FLAGS   ${CMAKE_CXX_FLAGS_RELEASE} )
	set_target_properties ( ${TEST_BIN_NAME_WITH_LTO} PROPERTIES COMPILE_FLAGS  "${CMAKE_CXX_FLAGS_RELEASE} -flto")
	set_target_properties ( ${TEST_BIN_NAME_WITH_LTO_WORKAROUND} PROPERTIES COMPILE_FLAGS  "${CMAKE_CXX_FLAGS_RELEASE} -flto")

endif (UNIX)

target_compile_options(${TEST_BIN_NAME} PRIVATE -pthread)
target_link_libraries(${TEST_BIN_NAME} ${LIB_NAME})

if(MSVC OR MSYS OR MINGW)
	set_target_properties(${TEST_BIN_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	set_target_properties(${TEST_BIN_NAME_WITH_LTO_WORKAROUND}  PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif(MSVC OR MSYS OR MINGW)

if(UNIX)
	add_test( safe_memory_test "${CMAKE_BINARY_DIR}/build/bin/${TEST_BIN_NAME}" )
	add_test( safe_memory_test_lto "${CMAKE_BINARY_DIR}/build/bin/${TEST_BIN_NAME_WITH_LTO}" )
	add_test( safe_memory_test_lto_wa "${CMAKE_BINARY_DIR}/build/bin/${TEST_BIN_NAME_WITH_LTO_WORKAROUND}" )
endif(UNIX)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TEST_BIN_NAME})