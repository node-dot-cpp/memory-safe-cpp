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

// class DiagHelper {
//   std::function<DiagnosticBuilder(SourceLocation, StringRef, DiagnosticIDs::Level)> Diag;
//   DiagnosticIDs::Level Level = DiagnosticIDs::Warning;
// public:

//   DiagHelper(ClangTidyCheck *Check, DiagnosticIDs::Level Level = DiagnosticIDs::Error)
//   :Diag(std::bind(&ClangTidyCheck::diag, Check, std::placeholders::_1,
//     std::placeholders::_2, std::placeholders::_3)), Level(Level) {}

//   DiagHelper(std::function<DiagnosticBuilder(SourceLocation, StringRef, DiagnosticIDs::Level)> Diag, DiagnosticIDs::Level Level = DiagnosticIDs::Error)
//   :Diag(Diag), Level(Level) {}

//   DiagHelper(ClangTidyContext* Context)
//   :Diag(std::bind(&ClangTidyContext::diagNote, Context, std::placeholders::_1,
//     std::placeholders::_2, std::placeholders::_3)), Level(DiagnosticIDs::Note) {}

//   DiagHelper() {}
  
//   void diag(SourceLocation Loc, StringRef Message) {
//     if(Diag) {
//       DiagnosticIDs::Level NextLevel = DiagnosticIDs::Note;
//       std::swap(Level, NextLevel);
//       Diag(Loc, Message, NextLevel);
//     }
//   }
// };

class DiagHelper {
  ClangTidyContext* Context = nullptr;
public:

  DiagHelper() {}
  DiagHelper(ClangTidyContext* Context) : Context(Context) {}

  void diag(clang::SourceLocation Loc, llvm::StringRef Message) const {
    if(Context)
      Context->diagNote(Loc, Message);
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
//bool isOsnPtrMethodName(const std::string& Name);
bool isOwningPtrName(const std::string& Name);
bool isSafePtrName(const std::string& Name);
bool isAwaitableName(const std::string &Name);
bool isNullablePtrName(const std::string& Name);

bool isSoftPtrCastName(const std::string& Name);
bool isWaitForAllName(const std::string& Name);
bool isNodeBaseName(const std::string& Name);
// bool isStdHashOrEqualToName(const std::string& Name);
bool isStdMoveOrForward(const std::string &Name);

bool isSystemLocation(const ClangTidyContext* Context, SourceLocation Loc);
bool isSystemSafeTypeName(const ClangTidyContext* Context, const std::string& Name);
bool isStackOnlyIteratorName(const ClangTidyContext *Context, const std::string &Name);
bool isSystemSafeFunction(const ClangTidyContext* Context, const std::string& Name);
bool isSystemSafeFunction(const FunctionDecl* Decl, const ClangTidyContext* Context);

std::string getQnameForSystemSafeDb(QualType Qt);
std::string getQnameForSystemSafeDb(const NamedDecl *Decl);

const CXXRecordDecl *getDeclWithDefinition(const CXXRecordDecl *Dc);

bool checkNakedStructRecord(const CXXRecordDecl *Dc, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);
KindCheck isNakedStructType(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);


enum class FunctionKind { None = 0, Lambda, StdFunction };

FunctionKind getFunctionKind(QualType Qt);

bool isStdFunctionType(QualType Qt);
bool isLambdaType(QualType Qt);

bool isRawPointerType(QualType Qt);
bool isNullPtrValue(ASTContext *Context, const Expr *Ex);

bool isStringLiteralType(QualType Qt);
bool isCharPointerType(QualType Qt);

QualType getPointeeType(QualType Qt);
QualType getTemplateArgType(QualType Qt, size_t i);

KindCheck isNullablePointerQtype(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);

// bool hasDeepConstAttr(const CXXRecordDecl* Decl);

bool templateArgIsSafe(QualType Qt, size_t i, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);
bool templateArgIsSafeAndDeepConst(QualType Qt, size_t i, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);
bool templateArgIsDeepConstAndNoSideEffectCallOp(QualType Qt, size_t i, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);

KindCheck isSafeVectorType(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);
KindCheck isSafeHashMapType(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);

bool isDeepConstOwningPtrType(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);
//bool isImplicitDeepConstStdHashOrEqualTo(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh = NullDiagHelper);

bool isSafePtrType(QualType Qt);
bool isAwaitableType(QualType Qt);
bool isNodecppErrorType(QualType Qt);

class TypeChecker {
  const ClangTidyContext* Context = nullptr;
  DiagHelper Dh;
  std::set<const CXXRecordDecl *> alreadyChecking;
  bool isSystemLoc = false;

  struct TypeRecursionRiia {
    TypeChecker& Tc;
    const CXXRecordDecl * current;
    bool alreadyHere = false;

    TypeRecursionRiia(TypeChecker& Tc, const CXXRecordDecl * current) :
      Tc(Tc), current(current) {
        alreadyHere = !Tc.alreadyChecking.insert(current).second;
      }

    ~TypeRecursionRiia() {
      if(!alreadyHere)
        Tc.alreadyChecking.erase(current);
    }

    bool wasAlreadyHere() const { return alreadyHere; }
  };
  friend struct TypeRecursionRiia;

  // std::set<const Type *> safeTypes;
  // std::set<const Type *> deterministicTypes;
  // std::set<const Type *> selfContainedTypes;

public:
  TypeChecker(const ClangTidyContext* Context) : Context(Context) {}
  TypeChecker(const ClangTidyContext* Context, const DiagHelper& Dh) :
    Context(Context), Dh(Dh) {}

  bool isSafeRecord(const CXXRecordDecl *Dc);
  bool isSafeType(const QualType& Qt);

  bool isStackOnlyIteratorRecord(const CXXRecordDecl *Dc);
  bool isStackOnlyIterator(const QualType& Qt);
  bool isStackOnlyQtype(const QualType& Qt);

  bool isDeterministicRecord(const CXXRecordDecl *Dc);
  bool isDeterministicType(const QualType& Qt);

  KindCheck isDeepConstRecord(const CXXRecordDecl *Dc);
  KindCheck isDeepConstType(QualType Qt);

  bool swapSystemLoc(bool newValue) {
    bool tmp = isSystemLoc;
    isSystemLoc = newValue;
    return tmp;
  }
};

inline
bool isSafeType(QualType Qt, const ClangTidyContext* Context,
  DiagHelper& Dh = NullDiagHelper) {

  TypeChecker Tc(Context, Dh);

  return Tc.isSafeType(Qt);
}

inline
bool isDeterministicType(QualType Qt, const ClangTidyContext* Context,
  DiagHelper& Dh = NullDiagHelper) {

  TypeChecker Tc(Context, Dh);

  return Tc.isDeterministicType(Qt);
}

inline
KindCheck isDeepConstType(QualType Qt, const ClangTidyContext* Context,
  DiagHelper& Dh = NullDiagHelper) {

  TypeChecker Tc(Context, Dh);

  return Tc.isDeepConstType(Qt);
}




const CXXRecordDecl* isUnionType(QualType Qt);
bool checkUnion(const CXXRecordDecl *Dc, DiagHelper& Dh = NullDiagHelper);

bool isOsnPtrRecord(const CXXRecordDecl *Dc);
const Expr* getBaseIfOsnPtrDerref(const Expr* Ex);

bool isImplicitExpr(const Expr *Ex);
const Expr *getParentExpr(ASTContext *Context, const Expr *Ex);
const Expr *ignoreTemporaries(const Expr *Ex);
const LambdaExpr *getLambda(const Expr *Ex);

/// \brief if this is a call to std::move then return its
/// argument, otherwise \c nullptr
DeclRefExpr *getStdMoveArg(Expr *Ex);

bool isFunctionPtr(const Expr *Ex);

const Stmt *getParentStmt(ASTContext *Context, const Stmt *St);
const DeclStmt* getParentDeclStmt(ASTContext *Context, const Decl* Dc);


/// \brief returns the enclosing \c FunctionDecl where this \c Stmt lives
const FunctionDecl* getEnclosingFunctionDecl(ASTContext *Context, const Stmt* St);

/// \brief returns the enclosing \c CXXRecordDecl where this \c Stmt lives
const CXXRecordDecl* getEnclosingCXXRecordDecl(ASTContext *Context, const Stmt* St);

/// \brief returns the enclosing \c CXXConstructorDecl where this \c Stmt lives
/// when \c Stmt is inside an initializer of such constructor
const CXXConstructorDecl* getEnclosingCXXConstructorDeclForInit(ASTContext *Context, const Stmt* St);


/// \brief Returns \c true if \p D is either a \c ParmVarDecl
/// or the argument of a \c CXXCatchStmt
bool isParmVarOrCatchVar(ASTContext *Context, const VarDecl *D);

/// \brief Returns \c true if this class is derived from NodeBase
bool isDerivedFromNodeBase(QualType Qt);

/// \brief Returns \c true if this is an empty class (without any members),
/// an all its base classes are also empty
bool isEmptyClass(QualType Qt);

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
