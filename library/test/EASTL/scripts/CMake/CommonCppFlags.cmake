#-------------------------------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------------------------------

# mb: we are using all this for testing only
if (MSVC AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    # emulating clang-cl specifics
    add_compile_options( -Wno-deprecated-declarations )
    add_compile_options( -Wno-ignored-pragma-optimize )
    add_compile_options( -Wno-return-type )
    add_compile_options( -Wno-reorder )
    add_compile_options( -Wno-microsoft-cast )
    add_compile_options( -Wno-unused-function )    
endif()
