set( LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  Support
  )

add_clang_executable(safememory-odr
  SafeMemoryOdr.cpp
  raiistdiofile.h
  )


target_link_libraries(safememory-odr
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  )

install(TARGETS safememory-odr
  RUNTIME DESTINATION bin)
