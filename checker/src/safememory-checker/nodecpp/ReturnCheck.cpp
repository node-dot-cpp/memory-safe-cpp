/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- ReturnCheck.cpp - clang-tidy------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ReturnCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void ReturnCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(returnStmt().bind("stmt"), this);
}

void ReturnCheck::check(const MatchFinder::MatchResult &Result) {
  auto St = Result.Nodes.getNodeAs<ReturnStmt>("stmt");

  auto Ex = St->getRetValue();
  if (!Ex)
    return;

  auto Fd = getEnclosingFunctionDecl(Result.Context, St);
  if(!Fd)
    return;

//  QualType Qt = Ex->getType().getCanonicalType();
  QualType Qt = Fd->getReturnType().getCanonicalType();

  // if(isRawPointerType(Qt) && isNullPtrValue(getASTContext(), Ex)) {
  //   diag(Ex->getExprLoc(), "(S1.3) raw pointer can't be null");
  //   return;
  // }
   
  if (getCheckHelper()->isStackOnly(Qt)) {

    auto Ch = NakedPtrScopeChecker::makeParamScopeChecker(this, getContext());
    if (!Ch.checkExpr(Ex))
      diag(Ex->getExprLoc(), "(S5.1) return value may extend scope");
  }

}

} // namespace checker
} // namespace nodecpp
