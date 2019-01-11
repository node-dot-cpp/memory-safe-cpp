set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(nodecppChecker
  ClangTidy.cpp
  ClangTidyModule.cpp
  ClangTidyDiagnosticConsumer.cpp
  ClangTidyOptions.cpp
  JSONSafeDatabase.cpp

  DEPENDS
  ClangSACheckers

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangFormat
  clangFrontend
  clangLex
  clangRewrite
  clangSema
  clangStaticAnalyzerCore
  clangStaticAnalyzerFrontend
  clangTooling
  clangToolingCore
  )

add_subdirectory(nodecpp)
add_subdirectory(tool)
