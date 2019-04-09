/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- CoroutineCheck.cpp - clang-tidy-----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CoroutineCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void CoroutineCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(coroutineBodyStmt(hasCoroutineBody(forEachDescendant(declRefExpr().bind("dref")))), this);
}

void CoroutineCheck::check(const MatchFinder::MatchResult &Result) {

  if(auto Dr = Result.Nodes.getNodeAs<DeclRefExpr>("dref")) {
    auto Qt = Dr->getDecl()->getType();
    if(isNakedPointerType(Qt.getCanonicalType(), getContext())) {
      diag(Dr->getExprLoc(), "(S5.8) naked pointer types not allowed inside coroutines");
      return;
    }
    else if(Qt->isReferenceType()) {
      diag(Dr->getExprLoc(), "(S5.8) references types not allowed inside coroutines");
      return;
    }
  }
}

} // namespace checker
} // namespace nodecpp
