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

namespace clang {
namespace tidy {
namespace nodecpp {

void NakedPtrReturnCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(returnStmt().bind("stmt"), this);
}

void NakedPtrReturnCheck::check(const MatchFinder::MatchResult &Result) {
  auto stmt = Result.Nodes.getNodeAs<ReturnStmt>("stmt");

  auto expr = stmt->getRetValue();
  if(!expr)
    return;

  QualType qt = expr->getType().getCanonicalType();
  if(isRawPointerType(qt)) {
    auto ch = NakedPtrScopeChecker::makeParamScopeChecker(this, getContext());
    if(!ch.checkExpr(expr)) 
        diag(expr->getExprLoc(), "(S5.1) return of raw pointer may extend scope");
  }
  else if(isNakedPointerType(qt, getContext())) {
    auto checker = NakedPtrScopeChecker::makeParamScopeChecker(this, getContext());

    if(!checker.checkExpr(expr))
        diag(expr->getExprLoc(), "(S5.1) return of naked pointer may extend scope");
  }
  else if(isNakedStructType(qt, getContext())) {
    auto checker = NakedPtrScopeChecker::makeParamScopeChecker(this, getContext());

    if(!checker.checkExpr(expr))
        diag(expr->getExprLoc(), "(S5.1) return of naked struct may extend scope");
  }



}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
