/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- NakedPtrHelper.h - clang-tidy------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NODECPP_NAKEDPTRHELPER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NODECPP_NAKEDPTRHELPER_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace nodecpp {


class DiagHelper {
  ClangTidyCheck *check = nullptr;
  DiagnosticIDs::Level level = DiagnosticIDs::Warning;
public:
  DiagHelper() {}
  DiagHelper(ClangTidyCheck *check)
  :check(check) {}

  void diag(SourceLocation loc, StringRef message) {
    if(check) {
      DiagnosticIDs::Level nextLevel = DiagnosticIDs::Note;
      std::swap(level, nextLevel);
      check->diag(loc, message, nextLevel);
    }
  }
};

extern DiagHelper NullDiagHelper;

class KindCheck {
  bool isKind = false;
  bool checkOk = false;
public:
  KindCheck(bool isKind, bool checkOk) :isKind(isKind), checkOk(checkOk) {}
  operator bool () const { return isKind; }
  bool isOk() const { return checkOk; }
};

bool isSystemLocation(const ClangTidyContext* context, SourceLocation loc);
bool isSystemSafeName(const ClangTidyContext* context, const std::string& name);

/// FIXME: Write a short description.
///
bool isOwnerPtrName(const std::string &Name);
bool isOwningPtrRecord(const CXXRecordDecl *decl);
bool isSafePtrName(const std::string &Name);

bool isNakedPtrName(const std::string& name);

bool isStdFunctionType(QualType qt);
bool isAnyFunctionType(QualType qt);

bool checkNakedStructRecord(const CXXRecordDecl *decl, const ClangTidyContext* context, DiagHelper& dh = NullDiagHelper);
KindCheck isNakedStructType(QualType qt, const ClangTidyContext* context, DiagHelper& dh = NullDiagHelper);



bool isLambdaType(QualType qt);
bool isNodecppFunctionOwnedArg0Type(QualType qt);

QualType getPointeeType(QualType qt);
KindCheck isNakedPointerType(QualType qt, const ClangTidyContext* context, DiagHelper& dh = NullDiagHelper);

bool isRawPointerType(QualType qt);
const ClassTemplateSpecializationDecl* getSafePtrDecl(QualType qt);
bool isSafePtrType(QualType qt);

bool isSafeRecord(const CXXRecordDecl *decl, const ClangTidyContext* context, DiagHelper& dh = NullDiagHelper);
bool isSafeType(QualType qt, const ClangTidyContext* context, DiagHelper& dh = NullDiagHelper);


const CXXRecordDecl* isUnionType(QualType qt);
bool checkUnion(const CXXRecordDecl *decl, DiagHelper& dh = NullDiagHelper);

bool isOsnPtrRecord(const CXXRecordDecl *decl);
const Expr* getBaseIfOsnPtrDerref(const Expr* expr);

const Expr *getParentExpr(ASTContext *context, const Expr *expr);
const Expr *ignoreTemporaries(const Expr *expr);
const LambdaExpr *getLambda(const Expr *expr);

bool isFunctionPtr(const Expr *expr);

const Stmt *getParentStmt(ASTContext *context, const Stmt *stmt);
const DeclStmt* getParentDeclStmt(ASTContext *context, const Decl* decl);

class NakedPtrScopeChecker {

  enum OutputScope { Unknown, Stack, Param, This };

  ClangTidyCheck *check; // to write diag messages
  ClangTidyContext* tidyContext;
  ASTContext *astContext;


  OutputScope outScope;
  const Decl* outScopeDecl; //only when outScope == Stack

  NakedPtrScopeChecker(ClangTidyCheck *check, ClangTidyContext* tidyContext, ASTContext *astContext, OutputScope outScope, const Decl* outScopeDecl) :
  check(check), tidyContext(tidyContext), astContext(astContext), outScope(outScope), outScopeDecl(outScopeDecl)  {}

  bool canArgumentGenerateOutput(QualType out, QualType arg);
  bool checkStack2StackAssignment(const Decl* fromDecl);

  bool checkDeclRefExpr(const DeclRefExpr *declRef);
  bool checkCallExpr(const CallExpr *call);
  bool checkCXXConstructExpr(const CXXConstructExpr *construct);

public:
  bool checkExpr(const Expr *from);
private:
  static
  std::pair<OutputScope, const Decl*> calculateScope(const Expr* expr);

  // static
  // std::pair<OutputScope, const Decl*> calculateShorterScope(std::pair<OutputScope, const Decl*> l, std::pair<OutputScope, const Decl*> r);

public:
  static
  NakedPtrScopeChecker makeChecker(ClangTidyCheck *check, ClangTidyContext* tidyContext, ASTContext *astContext, const Expr* toExpr);
  static
  NakedPtrScopeChecker makeThisScopeChecker(ClangTidyCheck *check, ClangTidyContext* tidyContext);
  static
  NakedPtrScopeChecker makeParamScopeChecker(ClangTidyCheck *check, ClangTidyContext* tidyContext);
};





} // namespace nodecpp
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NODECPP_NAKEDPTRHELPER_H
