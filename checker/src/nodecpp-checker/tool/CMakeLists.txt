set(LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  AllTargetsDescs
  AllTargetsInfos
  support
  )

add_clang_tool(nodecpp-checker
  ClangTidyMain.cpp
  )
add_dependencies(nodecpp-checker
  clang-headers
  )
target_link_libraries(nodecpp-checker
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  nodecppChecker
  clangTidyNodeCppModule
  clangTooling
  clangToolingCore
  )

