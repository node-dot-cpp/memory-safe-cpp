add_subdirectory(src)

# Add the testsuite after all the tools.
if(CLANG_INCLUDE_TESTS)
add_subdirectory(test)
endif()
