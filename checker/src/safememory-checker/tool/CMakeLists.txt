set(LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  Support
  )

add_clang_tool(safememory-checker
  ClangTidyMain.cpp
  )
add_dependencies(safememory-checker
  clang-headers
  )
target_link_libraries(safememory-checker
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  nodecppChecker
  clangTidyNodeCppModule
  clangTooling
  clangToolingCore
  )

