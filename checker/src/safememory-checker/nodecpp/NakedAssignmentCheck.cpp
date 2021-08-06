/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- NakedAssignmentCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NakedAssignmentCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void NakedAssignmentCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      cxxOperatorCallExpr(hasOverloadedOperatorName("=")
      ).bind("expr"), this);
}

void NakedAssignmentCheck::check(const MatchFinder::MatchResult &Result) {

  auto Ex = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("expr");

  if(isSystemLocation(getContext(), Ex->getExprLoc()))
    return;

  if (Ex->getNumArgs() == 2) {
    auto Left = Ex->getArg(0);
    auto Right = Ex->getArg(1);
    QualType Lqt = Left->getType().getCanonicalType();
    if (getCheckHelper()->isStackOnly(Left->getType())) {
      auto Checker = NakedPtrScopeChecker::makeChecker(this, getContext(),
                                                       Result.Context, Left);

      if (!Checker.checkExpr(Right))
        diag(Ex->getExprLoc(),
             "(S5.1) assignment may extend scope");
    // } else if (getCheckHelper()->isNullablePtr(Lqt)) {
    //   auto Checker = NakedPtrScopeChecker::makeChecker(this, getContext(),
    //                                                    Result.Context, Left);

    //   if (!Checker.checkExpr(Ex->getArg(1)))
    //     diag(Ex->getExprLoc(),
    //          "(S5.1) assignment of nullable_ptr may extend scope");
    // } else if (getCheckHelper()->isNakedStruct(Lqt)) {
    //   auto Checker = NakedPtrScopeChecker::makeChecker(this, getContext(), 
    //                                                    Result.Context, Left);

    //   if (!Checker.checkExpr(Ex->getArg(1)))
    //     diag(Ex->getExprLoc(),
    //          "(S5.1) assignment of naked_struct may extend scope");
    }
  }
}

} // namespace checker
} // namespace nodecpp
