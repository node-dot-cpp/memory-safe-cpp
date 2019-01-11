/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- ArrayExprCheck.cpp - clang-tidy-----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ArrayExprCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace nodecpp {

void ArrayExprCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(typeLoc(loc(arrayType())).bind("type"), this);

  Finder->addMatcher(
      arraySubscriptExpr()
          .bind("expr"),
      this);

}

void ArrayExprCheck::check(const MatchFinder::MatchResult &Result) {

  if(auto t = Result.Nodes.getNodeAs<TypeLoc>("type")) {
    diag(t->getLocStart(), "do not use arrays");
    return;
  }
  
  if(auto expr = Result.Nodes.getNodeAs<ArraySubscriptExpr>("expr")) {
    diag(expr->getRBracketLoc(), "do not use index operator on unsafe types");
    return;
  }
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
