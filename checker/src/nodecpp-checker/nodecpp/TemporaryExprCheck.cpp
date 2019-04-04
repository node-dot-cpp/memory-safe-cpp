/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- TemporaryExprCheck.cpp - clang-tidy-------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TemporaryExprCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void TemporaryExprCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(cxxTemporaryObjectExpr().bind("tmp"), this);
}

void TemporaryExprCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto Tmp = Result.Nodes.getNodeAs<CXXTemporaryObjectExpr>("tmp")) {

    QualType Qt = Tmp->getType().getCanonicalType();
    if (isSafeType(Qt, getContext()))
      return;
    if (isAnyFunctorType(Tmp->getType())) // don't use cannonical type here
      return;

    if (isRawPointerType(Qt))
      return;

    if (auto Np = isNakedPointerType(Qt, getContext())) {
      if (Np.isOk())
        return;

      auto Dh = DiagHelper(this);
      Dh.diag(Tmp->getExprLoc(), "unsafe type at temporary expression");
      isNakedPointerType(Qt, getContext(), Dh); // for report
      return;
    }

    if (auto Ns = isNakedStructType(Qt, getContext())) {
      if (Ns.isOk())
        return;

      auto Dh = DiagHelper(this);
      Dh.diag(Tmp->getExprLoc(), "unsafe type at temporary expression");
      isNakedStructType(Qt, getContext(), Dh); // for report
      return;
    }

    if (isLambdaType(Qt))
      return;

//    tmp->dump();
auto Dh = DiagHelper(this);
Dh.diag(Tmp->getExprLoc(), "unsafe type at temporary expression");
isSafeType(Qt, getContext(), Dh);
  }
}

} // namespace checker
} // namespace nodecpp
