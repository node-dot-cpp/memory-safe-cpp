set( LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  Support
  )

add_clang_executable(safememory-library-db
  NodeCppSafeLibrary.cpp
  raiistdiofile.h
  )


target_link_libraries(safememory-library-db
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  )

install(TARGETS safememory-library-db
  RUNTIME DESTINATION bin)
