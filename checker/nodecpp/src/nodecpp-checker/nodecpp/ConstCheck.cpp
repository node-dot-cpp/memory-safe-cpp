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

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace nodecpp {

void ConstCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(cxxConstCastExpr().bind("cE"), this);
  Finder->addMatcher(fieldDecl().bind("fD"), this);
  Finder->addMatcher(lambdaExpr().bind("lE"), this);
}

void ConstCheck::check(const MatchFinder::MatchResult &Result) {

  if(auto cE = Result.Nodes.getNodeAs<CXXConstCastExpr>("cE")) {
    diag(cE->getExprLoc(), "(S2.1) const_cast is prohibited");
  }
  else if(auto fD = Result.Nodes.getNodeAs<FieldDecl>("fD")) {
    if(fD->isMutable()) {
      diag(fD->getLocation(), "(S2.2) mutable members are prohibited");
    }
  }
  else if(auto lE = Result.Nodes.getNodeAs<LambdaExpr>("lE")) {
    if(lE->isMutable()) {
      diag(lE->getExprLoc(), "(S2) mutable lambdas are prohibited");
    }
  }
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
