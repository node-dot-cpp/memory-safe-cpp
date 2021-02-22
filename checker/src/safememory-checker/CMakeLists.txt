set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(nodecppChecker
  CheckerData.cpp
  ClangTidy.cpp
  ClangTidyModule.cpp
  ClangTidyDiagnosticConsumer.cpp
  ClangTidyOptions.cpp
  ConsistencyRule.cpp
  DeterminismRule.cpp
  JSONSafeDatabase.cpp
  MiscellaneousRule.cpp
  MustCoAwaitRule.cpp
  NoSideEffectRule.cpp
  ReferenceOverCoAwaitRule.cpp

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

if (MSVC)
  set_source_files_properties(ClangTidy.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

add_subdirectory(nodecpp)
add_subdirectory(tool)
