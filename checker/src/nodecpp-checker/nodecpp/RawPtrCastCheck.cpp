/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- RawPtrCastCheck.cpp - clang-tidy----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RawPtrCastCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace nodecpp {

void RawPtrCastCheck::registerMatchers(MatchFinder *Finder) {
/*
  Finder->addMatcher(cxxStaticCastExpr(hasDestinationType(pointerType())).bind("cast"), this);
  Finder->addMatcher(cxxStaticCastExpr( hasDestinationType(lValueReferenceType()) ).bind("cast"), this);
  	
  Finder->addMatcher(cxxReinterpretCastExpr(hasDestinationType(pointerType())).bind("cast"), this);
  Finder->addMatcher(cxxReinterpretCastExpr(hasDestinationType(referenceType())).bind("cast"), this);
  
  Finder->addMatcher(cStyleCastExpr(hasDestinationType(pointerType())).bind("cast"), this);
  Finder->addMatcher(cStyleCastExpr(hasDestinationType(referenceType())).bind("cast"), this);
*/
}

void RawPtrCastCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *MatchedCast = Result.Nodes.getNodeAs<ExplicitCastExpr>("cast");
  
  diag(MatchedCast->getExprLoc(), "(S1.1) casts are prohibited");
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
