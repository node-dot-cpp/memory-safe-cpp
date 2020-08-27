#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------

# mb: this is not the real root CMakeLists.txt for this project.
# this project requires to be built as part of llvm/clang build,
# as an llvm 'external' project.
# This file will be processed from inside clang, as an external tool.

add_subdirectory(src)

# Add the testsuite after all the tools.
if(CLANG_INCLUDE_TESTS)
    add_subdirectory(test)
endif()
