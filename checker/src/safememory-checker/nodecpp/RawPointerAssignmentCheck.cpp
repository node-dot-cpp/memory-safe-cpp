/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- RawPointerAssignmentCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RawPointerAssignmentCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void RawPointerAssignmentCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      binaryOperator(hasOperatorName("="),
          hasType(pointerType())/*,
          unless(hasLHS(ignoringImpCasts(declRefExpr(to(isImplicit())))))*/)
          .bind("expr"),
      this);
}

void RawPointerAssignmentCheck::check(const MatchFinder::MatchResult &Result) {

  auto Ex = Result.Nodes.getNodeAs<BinaryOperator>("expr");

  if(isSystemLocation(getContext(), Ex->getExprLoc()))
    return;

  auto Checker = NakedPtrScopeChecker::makeChecker(
      this, getContext(), Result.Context, Ex->getLHS());

  if (!Checker.checkExpr(Ex->getRHS())) {
    diag(Ex->getExprLoc(),
         "(S5.1) assignment of raw pointer may extend scope");
    return;
  }

  // if(isNullPtrValue(getASTContext(), Ex->getRHS())) {
  //   diag(Ex->getExprLoc(),
  //        "(S1.3) raw pointer variable can't be assigned to null");
  //   return;
  // }

  
}

} // namespace checker
} // namespace nodecpp
