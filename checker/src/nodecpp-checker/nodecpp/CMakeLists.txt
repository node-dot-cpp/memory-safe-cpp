set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyNodeCppModule
  ArrayExprCheck.cpp
  AsmCheck.cpp
  CallExprCheck.cpp
  ConstCheck.cpp
  CoroutineCheck.cpp
  MayExtendLambdaCheck.cpp
  NakedAssignmentCheck.cpp
  NakedPtrHelper.cpp
  NewExprCheck.cpp
  NodeCppTidyModule.cpp
  RawPointerAssignmentCheck.cpp
  RawPtrExprCheck.cpp
  RecordDeclCheck.cpp
  ReturnCheck.cpp
  StaticStorageCheck.cpp
  StringLiteralCheck.cpp
  TemporaryExprCheck.cpp
  VarDeclCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTooling
  )
