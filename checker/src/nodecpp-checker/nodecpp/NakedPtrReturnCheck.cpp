/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- NakedPtrReturnCheck.cpp - clang-tidy------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NakedPtrReturnCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void NakedPtrReturnCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(returnStmt().bind("stmt"), this);
}

void NakedPtrReturnCheck::check(const MatchFinder::MatchResult &Result) {
  auto St = Result.Nodes.getNodeAs<ReturnStmt>("stmt");

  auto Ex = St->getRetValue();
  if (!Ex)
    return;

  QualType Qt = Ex->getType().getCanonicalType();
  if (isRawPointerType(Qt)) {
    auto Ch = NakedPtrScopeChecker::makeParamScopeChecker(this, getContext());
    if (!Ch.checkExpr(Ex))
      diag(Ex->getExprLoc(), "(S5.1) return of raw pointer may extend scope");
  } else if (isNakedPointerType(Qt, getContext())) {
    auto Checker =
        NakedPtrScopeChecker::makeParamScopeChecker(this, getContext());

    if (!Checker.checkExpr(Ex))
      diag(Ex->getExprLoc(),
           "(S5.1) return of naked pointer may extend scope");
  } else if (isNakedStructType(Qt, getContext())) {
    auto Checker =
        NakedPtrScopeChecker::makeParamScopeChecker(this, getContext());

    if (!Checker.checkExpr(Ex))
      diag(Ex->getExprLoc(),
           "(S5.1) return of naked struct may extend scope");
  }
}

} // namespace checker
} // namespace nodecpp
