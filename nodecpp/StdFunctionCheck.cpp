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

namespace clang {
namespace tidy {
namespace nodecpp {

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

void StdFunctionCheck::checkLambda(QualType qt, bool ownedArg0, SourceLocation callLoc) {

  assert(qt.isCanonical());
  auto decl = qt->getAsCXXRecordDecl();
  assert(decl);
  assert(decl->hasDefinition());
  assert(decl->isLambda());
  auto m = decl->getLambdaCallOperator();

  for (unsigned i = 0; i != m->param_size(); ++i) {
    auto p = m->getParamDecl(i);
    if (p->hasAttr<NodeCppMayExtendAttr>()) {
      diag(callLoc, "lambda with attribute [[may_extend_to_this]] can't be used to initialize object");
      diag(p->getLocation(), "declared here", DiagnosticIDs::Note);
      return;
    }
    
    if(ownedArg0 && i == 0) {
      if(!p->hasAttr<NodeCppOwnedByThisAttr>()) {
        diag(callLoc, "lambda without attribute [[owned_by_this]] can't be used to initialize object");
        diag(p->getLocation(), "referenced from here", DiagnosticIDs::Note);
        return;
      }
    }
    else {
        if(p->hasAttr<NodeCppOwnedByThisAttr>()) {
        diag(callLoc, "lambda with attribute [[owned_by_this]] can't be used to initialize object");
        diag(p->getLocation(), "declared here", DiagnosticIDs::Note);
        return;
      }
    }
  }
}

void StdFunctionCheck::checkFunctions(QualType arg0, QualType arg1, SourceLocation callLoc) {
  
  assert(arg0.isCanonical());
  assert(arg1.isCanonical());
  
  if(isStdFunctionType(arg0)) {
    if(isStdFunctionType(arg1)) {
      // this is ok
      return;
    }
    else if(isLambdaType(arg1)) {
      // check it doesn't have attributes
      checkLambda(arg1, false, callLoc);
      //is ok!
    }
    else {
      // this is forbidden
      diag(callLoc, "std::function can be assigned from lambda or from other std::function");
      return;
    }
  }
  else if(isNodecppFunctionOwnedArg0Type(arg0)) {
    if(isNodecppFunctionOwnedArg0Type(arg1)) {
      // this is ok
      return;
    }
    else if(isLambdaType(arg1)) {
      // check it does have attributes
      checkLambda(arg1, true, callLoc);
    }
    else {
      // this is forbidden
      diag(callLoc, "nodecpp::function_with_owned can be assigned from lambda or from other nodecpp::function_with_owned");
      return;
    }
  }
}

void StdFunctionCheck::check(const MatchFinder::MatchResult &Result) {

  if(auto opAsgn = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("op")) {
    if(opAsgn->getNumArgs() >= 2) {
      QualType arg0 = opAsgn->getArg(0)->getType().getCanonicalType();
      QualType arg1 = opAsgn->getArg(1)->getType().getCanonicalType();

      checkFunctions(arg0, arg1, opAsgn->getExprLoc());
    }
  }
  else if(auto ctor = Result.Nodes.getNodeAs<CXXConstructExpr>("ctor")) {
    if(ctor->getNumArgs() >= 1) {
      QualType arg0 = ctor->getType().getCanonicalType();
      QualType arg1 = ctor->getArg(0)->getType().getCanonicalType();

      checkFunctions(arg0, arg1, ctor->getExprLoc());
    }
  }
  else if(auto opCall = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("call")) {
    if(opCall->getNumArgs() >= 1) {
      QualType arg0 = opCall->getArg(0)->getType().getCanonicalType();
      if(isNodecppFunctionOwnedArg0Type(arg0)) {
        diag(opCall->getExprLoc(), "function with attribute [[owned_by_this]] can't called from safe code");
      }
    }
  }
  else if(auto mAssign = Result.Nodes.getNodeAs<CXXMemberCallExpr>("assg")) {
    if(mAssign->getNumArgs() >= 1) {
      QualType arg0 = mAssign->getImplicitObjectArgument()->getType().getCanonicalType();
      QualType arg1 = mAssign->getArg(0)->getType().getCanonicalType();

      checkFunctions(arg0, arg1, mAssign->getExprLoc());
    }
  }
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
