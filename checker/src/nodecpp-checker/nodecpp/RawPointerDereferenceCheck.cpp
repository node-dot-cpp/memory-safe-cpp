/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- RawPointerDereferenceCheck.cpp - clang-tidy-----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RawPointerDereferenceCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void RawPointerDereferenceCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(memberExpr(isArrow()).bind("arrow"), this);

  Finder->addMatcher(unaryOperator(hasOperatorName("*"),
                                   hasUnaryOperand(hasType(pointerType())))
                         .bind("star"),
                     this);
}

void RawPointerDereferenceCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto Ex = Result.Nodes.getNodeAs<MemberExpr>("arrow")) {
    auto Base = Ex->getBase()->IgnoreParenImpCasts();
    if (isa<CXXThisExpr>(Base))
      return;
    else if (isa<CallExpr>(Base))
      return;

    diag(Ex->getExprLoc(),
         "(S1.2) dereference of raw pointers is prohibited");
    return;
  } else if (auto Ex = Result.Nodes.getNodeAs<UnaryOperator>("star")) {
    auto Base = Ex->getSubExpr()->IgnoreParenImpCasts();
    if (isa<CXXThisExpr>(Base))
      return;
    else if (isa<CallExpr>(Base))
      return;

    diag(Ex->getExprLoc(),
         "(S1.2) dereference of raw pointers is prohibited");
  }
}

} // namespace checker
} // namespace nodecpp
