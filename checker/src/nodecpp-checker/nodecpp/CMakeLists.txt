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
  NakedPtrFuncCheck.cpp
  NakedPtrReturnCheck.cpp
  NewExprCheck.cpp
  NodeCppTidyModule.cpp
  PtrArithmeticCheck.cpp
  RawPointerAssignmentCheck.cpp
  RawPointerDereferenceCheck.cpp
  RawPtrExprCheck.cpp
  RecordDeclCheck.cpp
  StaticStorageCheck.cpp
  TemporaryExprCheck.cpp
  VarDeclCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyMiscModule
  clangTidyUtils
  clangTooling
  )
