set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(nodecppChecker
  CheckerData.cpp
  ClangTidy.cpp
  ClangTidyModule.cpp
  ClangTidyDiagnosticConsumer.cpp
  ClangTidyOptions.cpp
  JSONSafeDatabase.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangFormat
  clangFrontend
  clangLex
  clangRewrite
  clangSema
  clangTooling
  clangToolingCore
  )

add_subdirectory(nodecpp)
add_subdirectory(tool)
