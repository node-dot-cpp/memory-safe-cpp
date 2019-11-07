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

#ifndef NODECPP_CHECKER_NODECPP_NAKEDPTRHELPER_H
#define NODECPP_CHECKER_NODECPP_NAKEDPTRHELPER_H

#include "../ClangTidy.h"

namespace nodecpp {
namespace checker {

extern const char *DiagMsgSrc;

class DiagHelper {
  ClangTidyCheck *Check = nullptr;
  DiagnosticIDs::Level Level = DiagnosticIDs::Warning;
public:

  DiagHelper(ClangTidyCheck *Check, DiagnosticIDs::Level Level = DiagnosticIDs::Error)
  :Check(Check), Level(Level) {}

  void diag(SourceLocation Loc, StringRef Message) {
    if(Check) {
      DiagnosticIDs::Level NextLevel = DiagnosticIDs::Note;
      std::swap(Level, NextLevel);
      Check->diag(Loc, Message, NextLevel);
    }
  }
};

extern DiagHelper NullDiagHelper;

class KindCheck {
  bool IsKind = false;
  bool CheckOk = false;
public:
  KindCheck(bool IsKind, bool CheckOk) :IsKind(IsKind), CheckOk(CheckOk) {}
  operator bool () const { return IsKind; }
  bool isOk() const { return CheckOk; }
};


/// FIXME: Write a short description.
///
bool isOwnerPtrName(const std::string& Name);
bool isOwnerPtrDecl(const NamedDecl* Dc);
bool isSafePtrName(const std::string& Name);
bool isAwaitableName(const std::string &Name);

bool isNakedPtrName(const std::string& Name);
bool isConstNakedPtrName(const std::string& Name);

bool isSystemLocation(const ClangTidyContext* Context, SourceLocation Loc);
bool isSystemSafeTypeName(const ClangTidyContext* Context, const std::string& Name);
bool isSystemSafeFunctionName(const ClangTidyContext* Context, const std::string& Name);

bool checkNakedStructRecord(const CXXRecordDecl *Dc, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);
KindCheck isNakedStructType(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);


enum class FunctionKind { None = 0, Lambda, StdFunction, OwnedArg0 };

FunctionKind getFunctionKind(QualType Qt);

bool isStdFunctionType(QualType Qt);
bool isLambdaType(QualType Qt);
bool isNodecppFunctionOwnedArg0Type(QualType Qt);
bool isAnyFunctorType(QualType Qt);

bool isRawPointerType(QualType Qt);
const ClassTemplateSpecializationDecl* getTemplatePtrDecl(QualType Qt);

QualType getPointeeType(QualType Qt);
KindCheck isNakedPointerType(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);
bool isConstNakedPointerType(QualType Qt);
bool isSafePtrType(QualType Qt);
bool isAwaitableType(QualType Qt);

class TypeChecker {
  const ClangTidyContext* Context = nullptr;
  DiagHelper Dh = {nullptr};
  std::set<const CXXRecordDecl *> alreadyChecking;

public:
  TypeChecker(const ClangTidyContext* Context) : Context(Context) {}
  TypeChecker(const ClangTidyContext* Context, ClangTidyCheck *Check) :
    Context(Context), Dh(Check) {}
  TypeChecker(const ClangTidyContext* Context, const DiagHelper& Dh) :
    Context(Context), Dh(Dh) {}

  bool isSafeRecord(const CXXRecordDecl *Dc);
  bool isSafeType(const QualType& Qt);
};

inline
bool isSafeRecord(const CXXRecordDecl *Dc, const ClangTidyContext* Context,
  DiagHelper& Dh = NullDiagHelper) {

  TypeChecker Tc(Context, Dh);

  return Tc.isSafeRecord(Dc);
}

inline
bool isSafeType(QualType Qt, const ClangTidyContext* Context,
  DiagHelper& Dh = NullDiagHelper) {

  TypeChecker Tc(Context, Dh);

  return Tc.isSafeType(Qt);
}


const CXXRecordDecl* isUnionType(QualType Qt);
bool checkUnion(const CXXRecordDecl *Dc, DiagHelper& Dh = NullDiagHelper);

bool isOsnPtrRecord(const CXXRecordDecl *Dc);
const Expr* getBaseIfOsnPtrDerref(const Expr* Ex);

bool isImplicitExpr(const Expr *Ex);
const Expr *getParentExpr(ASTContext *Context, const Expr *Ex);
const Expr *ignoreTemporaries(const Expr *Ex);
const LambdaExpr *getLambda(const Expr *Ex);

bool isFunctionPtr(const Expr *Ex);

const Stmt *getParentStmt(ASTContext *Context, const Stmt *St);
const DeclStmt* getParentDeclStmt(ASTContext *Context, const Decl* Dc);

class NakedPtrScopeChecker {

  enum OutputScope { Unknown, Stack, Param, This };

  ClangTidyCheck *Check; // to write diag messages
  ClangTidyContext *TidyContext;
  ASTContext *AstContext;


  OutputScope OutScope;
  const Decl *OutScopeDecl; //only when outScope == Stack

  NakedPtrScopeChecker(ClangTidyCheck *Check, ClangTidyContext* TidyContext, ASTContext *AstContext, OutputScope OutScope, const Decl* OutScopeDecl) :
  Check(Check), TidyContext(TidyContext), AstContext(AstContext), OutScope(OutScope), OutScopeDecl(OutScopeDecl)  {}

  bool canArgumentGenerateOutput(QualType Out, QualType Arg);
  bool checkStack2StackAssignment(const Decl* FromDecl);

  bool checkDeclRefExpr(const DeclRefExpr *DeclRef);
  bool checkCallExpr(const CallExpr *Call);
  bool checkCXXConstructExpr(const CXXConstructExpr *Construct);

public:
  bool checkExpr(const Expr *From);
private:
  static
  std::pair<OutputScope, const Decl*> calculateScope(const Expr* Ex);

  // static
  // std::pair<OutputScope, const Decl*> calculateShorterScope(std::pair<OutputScope, const Decl*> l, std::pair<OutputScope, const Decl*> r);

public:
  static
  NakedPtrScopeChecker makeChecker(ClangTidyCheck *Check, ClangTidyContext* TidyContext, ASTContext *AstContext, const Expr* ToExpr);
  static
  NakedPtrScopeChecker makeThisScopeChecker(ClangTidyCheck *Check, ClangTidyContext* TidyContext);
  static
  NakedPtrScopeChecker makeParamScopeChecker(ClangTidyCheck *Check, ClangTidyContext* TidyContext);
};

} // namespace checker
} // namespace nodecpp

#endif // NODECPP_CHECKER_NODECPP_NAKEDPTRHELPER_H
