/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- StdFunctionCheck.cpp - clang-tidy---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "StdFunctionCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void StdFunctionCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(
      cxxOperatorCallExpr(hasOverloadedOperatorName("=")
      ).bind("op"), this);

  Finder->addMatcher(
      cxxConstructExpr().bind("ctor"), this);

  Finder->addMatcher(
      cxxOperatorCallExpr(hasOverloadedOperatorName("()")
      ).bind("call"), this);

  Finder->addMatcher(
      cxxMemberCallExpr(callee(cxxMethodDecl(hasName("assign")))).bind("assg"),
      this);
}

void StdFunctionCheck::checkLambda(QualType Qt, bool OwnedArg0,
                                   SourceLocation CallLoc) {

  assert(Qt.isCanonical());
  auto Decl = Qt->getAsCXXRecordDecl();
  assert(Decl);
  assert(Decl->hasDefinition());
  assert(Decl->isLambda());
  auto M = Decl->getLambdaCallOperator();

  for (unsigned I = 0; I != M->param_size(); ++I) {
    auto P = M->getParamDecl(I);
    if (P->hasAttr<NodeCppMayExtendAttr>()) {
      diag(CallLoc, "lambda with attribute [[may_extend_to_this]] can't be "
                    "used to initialize object");
      diag(P->getLocation(), "declared here", DiagnosticIDs::Note);
      return;
    }

    if (OwnedArg0 && I == 0) {
      if (!P->hasAttr<NodeCppOwnedByThisAttr>()) {
        diag(CallLoc, "lambda without attribute [[owned_by_this]] can't be "
                      "used to initialize object");
        diag(P->getLocation(), "referenced from here", DiagnosticIDs::Note);
        return;
      }
    } else {
      if (P->hasAttr<NodeCppOwnedByThisAttr>()) {
        diag(CallLoc, "lambda with attribute [[owned_by_this]] can't be used "
                      "to initialize object");
        diag(P->getLocation(), "declared here", DiagnosticIDs::Note);
        return;
      }
    }
  }
}

void StdFunctionCheck::checkFunctions(QualType Arg0Sugar, QualType Arg1Sugar,
                                      SourceLocation CallLoc) {

//  QualType Arg0 = Arg0Sugar.getCanonicalType();
  QualType Arg1 = Arg1Sugar.getCanonicalType();

  //first check the alias
  if (isNodecppFunctionOwnedArg0Type(Arg0Sugar)) {
    if (isNodecppFunctionOwnedArg0Type(Arg1Sugar)) {
      // this is ok
      return;
    } else if (isLambdaType(Arg1Sugar)) {
      // check it does have attributes
      checkLambda(Arg1, true, CallLoc);
    } else {
      // this is forbidden
      diag(CallLoc, "nodecpp::function_with_owned can be assigned from lambda "
                    "or from other nodecpp::function_with_owned");
      return;
    }
  } else if (isStdFunctionType(Arg0Sugar)) {
    if (isStdFunctionType(Arg1Sugar)) {
      // this is ok
      return;
    } else if (isLambdaType(Arg1Sugar)) {
      // check it doesn't have attributes
      checkLambda(Arg1, false, CallLoc);
      //is ok!
    } else {
      // this is forbidden
      diag(CallLoc, "std::function can be assigned from lambda or from other "
                    "std::function");
      return;
    }
  }
}

void StdFunctionCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto OpAsgn = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("op")) {
    if (OpAsgn->getNumArgs() >= 2) {
      QualType Arg0 = OpAsgn->getArg(0)->getType();
      QualType Arg1 = OpAsgn->getArg(1)->getType();

      checkFunctions(Arg0, Arg1, OpAsgn->getExprLoc());
    }
  } else if (auto Ctor = Result.Nodes.getNodeAs<CXXConstructExpr>("ctor")) {
    if (Ctor->getNumArgs() >= 1) {
      QualType Arg0 = Ctor->getType();
      QualType Arg1 = Ctor->getArg(0)->getType();

      checkFunctions(Arg0, Arg1, Ctor->getExprLoc());
    }
  } else if (auto OpCall =
                 Result.Nodes.getNodeAs<CXXOperatorCallExpr>("call")) {
    if (OpCall->getNumArgs() >= 1) {
      QualType Arg0 = OpCall->getArg(0)->getType();
      if (isNodecppFunctionOwnedArg0Type(Arg0)) {
        diag(OpCall->getExprLoc(), "function with attribute [[owned_by_this]] "
                                   "can't called from safe code");
      }
    }
  } else if (auto MAssign = Result.Nodes.getNodeAs<CXXMemberCallExpr>("assg")) {
    if (MAssign->getNumArgs() >= 1) {
      QualType Arg0 = MAssign->getImplicitObjectArgument()->getType();
      QualType Arg1 = MAssign->getArg(0)->getType();

      checkFunctions(Arg0, Arg1, MAssign->getExprLoc());
    }
  }
}

} // namespace checker
} // namespace nodecpp
