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

namespace clang {
namespace tidy {
namespace nodecpp {

void RawPointerDereferenceCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(memberExpr(isArrow()).bind("arrow"), this);

  Finder->addMatcher(unaryOperator(hasOperatorName("*"),
                                   hasUnaryOperand(hasType(pointerType())))
                         .bind("star"),
                     this);
}

void RawPointerDereferenceCheck::check(const MatchFinder::MatchResult &Result) {

  if(auto expr = Result.Nodes.getNodeAs<MemberExpr>("arrow")) {
    auto base = expr->getBase()->IgnoreParenImpCasts();
    if(isa<CXXThisExpr>(base))
      return;
    else if(isa<CallExpr>(base))
      return;

    diag(expr->getExprLoc(), "(S1.2) dereference of raw pointers is prohibited");
    return;
  }
  else if(auto expr = Result.Nodes.getNodeAs<UnaryOperator>("star")) {

    diag(expr->getExprLoc(), "(S1.2) dereference of raw pointers is prohibited");
  }
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
