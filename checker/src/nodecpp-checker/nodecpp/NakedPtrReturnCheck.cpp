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

  auto Fd = getParentFunctionDecl(Result.Context, St);
  if(!Fd)
    return;

//  QualType Qt = Ex->getType().getCanonicalType();
  QualType Qt = Fd->getReturnType().getCanonicalType();

  if (isRawPointerType(Qt) || isNakedPointerType(Qt, getContext()) ||
    isNakedStructType(Qt, getContext()) || Qt->isLValueReferenceType()) {

    auto Ch = NakedPtrScopeChecker::makeParamScopeChecker(this, getContext());
    if (!Ch.checkExpr(Ex))
      diag(Ex->getExprLoc(), "(S5.1) return value may extend scope");
  }
}

} // namespace checker
} // namespace nodecpp
