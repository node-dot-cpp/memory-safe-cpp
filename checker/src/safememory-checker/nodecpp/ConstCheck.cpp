/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- ConstCheck.cpp - clang-tidy---------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ConstCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "NakedPtrHelper.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void ConstCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(cxxConstCastExpr().bind("cE"), this);
  Finder->addMatcher(fieldDecl().bind("fD"), this);
  Finder->addMatcher(lambdaExpr().bind("lE"), this);
}

void ConstCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto CE = Result.Nodes.getNodeAs<CXXConstCastExpr>("cE")) {

    if(isSystemLocation(getContext(), CE->getExprLoc()))
      return;

    diag(CE->getExprLoc(), "(S2.1) const_cast is prohibited");
  } else if (auto FD = Result.Nodes.getNodeAs<FieldDecl>("fD")) {

    if(isSystemLocation(getContext(), FD->getLocation()))
      return;


    if (FD->isMutable()) {
      diag(FD->getLocation(), "(S2.2) mutable members are prohibited");
    }
  // } else if (auto LE = Result.Nodes.getNodeAs<LambdaExpr>("lE")) {
  //   if (LE->isMutable()) {
  //     diag(LE->getExprLoc(), "(S2) mutable lambdas are prohibited");
  //   }
  }
}

} // namespace checker
} // namespace nodecpp
