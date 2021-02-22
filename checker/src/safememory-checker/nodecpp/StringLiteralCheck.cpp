/*******************************************************************************
  Copyright (C) 2020 OLogN Technologies AG
*******************************************************************************/
//===--- StringLiteralCheck.cpp - clang-tidy---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "StringLiteralCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void StringLiteralCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(
      cxxOperatorCallExpr(hasOverloadedOperatorName("=")
      ).bind("op="), this);

  Finder->addMatcher(
      cxxConstructExpr().bind("ctor"), this);
}

void StringLiteralCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto OpAsgn = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("op=")) {
    if (OpAsgn->getNumArgs() >= 2) {
      QualType Arg0 = OpAsgn->getArg(0)->getType().getCanonicalType();
      QualType Arg1 = OpAsgn->getArg(1)->getType().getCanonicalType();
      if(isStringLiteralType(Arg0) && isCharPointerType(Arg1)) {
        const Expr *E = OpAsgn->getArg(1)->IgnoreParenImpCasts();
        if(!isa<clang::StringLiteral>(E)) {
          diag(OpAsgn->getExprLoc(), "(S10.1) string_literal assignment not allowed");

        }
      }
    }
  } else if (auto Ctor = Result.Nodes.getNodeAs<CXXConstructExpr>("ctor")) {
    if (Ctor->getNumArgs() >= 1) {
      QualType Arg0 = Ctor->getType().getCanonicalType();
      QualType Arg1 = Ctor->getArg(0)->getType().getCanonicalType();

      if(isStringLiteralType(Arg0) && isCharPointerType(Arg1)) {
        const Expr *E = Ctor->getArg(0)->IgnoreParenImpCasts();
        if(!isa<clang::StringLiteral>(E)) {
          diag(Ctor->getExprLoc(), "(S10.1) string_literal constructor not allowed");
        }
      }
    }
  }
}

} // namespace checker
} // namespace nodecpp
