/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- MayExtendDeclCheck.cpp - clang-tidy-------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MayExtendDeclCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void MayExtendDeclCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(functionDecl(hasAnyParameter(hasAttr(
                                           attr::NodeCppMayExtend))).bind("decl"),
                     this);
  Finder->addMatcher(functionDecl(hasAnyParameter(hasAttr(
                                           attr::NodeCppOwnedByThis))).bind("decl"),
                     this);

  Finder->addMatcher(lambdaExpr().bind("lamb"), this);
}

void MayExtendDeclCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto L = Result.Nodes.getNodeAs<LambdaExpr>("lamb")) {
    auto Decl = L->getCallOperator();
    for (unsigned I = 0; I != Decl->param_size(); ++I) {
      auto P = Decl->getParamDecl(I);
      if (P->hasAttr<NodeCppMayExtendAttr>()) {
        diag(P->getLocation(), "attribute can't be used on lambdas");
        return;
      }
    }
    //this lambda is ok
    return;
  } else if (auto D = Result.Nodes.getNodeAs<FunctionDecl>("decl")) {

    auto M = dyn_cast<CXXMethodDecl>(D);
    if (!M) {
      diag(D->getLocation(),
           "attribute can only be used on non-static member methods");
      return;
    }

    if (M->isStatic()) {
      diag(D->getLocation(),
           "attribute can't be used on static member methods");
      return;
    } else if (isLambdaCallOperator(M)) {
      //this is not matching anything
      diag(D->getLocation(), "attribute can't be used on lambdas");
      return;
    }
    //this is ok
  }
}

} // namespace checker
} // namespace nodecpp
