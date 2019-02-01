set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

set(LLVM_REQUIRES_RTTI 1)

add_clang_executable(nodecpp-safe-library
  NodeCppSafeLibrary.cpp
  raiistdiofile.h
  )


target_link_libraries(nodecpp-safe-library
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangStaticAnalyzerFrontend
  clangTooling
  )

install(TARGETS nodecpp-safe-library
  RUNTIME DESTINATION bin)
