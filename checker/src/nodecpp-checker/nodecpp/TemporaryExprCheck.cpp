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
  Finder->addMatcher(materializeTemporaryExpr().bind("tmp2"), this);
}

void TemporaryExprCheck::check(const MatchFinder::MatchResult &Result) {

  QualType Qt;
  SourceLocation Loc;
  if (auto Tmp = Result.Nodes.getNodeAs<CXXTemporaryObjectExpr>("tmp")) {
    Qt = Tmp->getType();
    Loc = Tmp->getExprLoc();
  }

  else if (auto Tmp2 = Result.Nodes.getNodeAs<MaterializeTemporaryExpr>("tmp2")) {
    Qt = Tmp2->getType();
    Loc = Tmp2->getExprLoc();
  }

  if (!Qt.isNull()) {

    Qt = Qt.getCanonicalType();
    if (isStdFunctionType(Qt))
      return;

    if (isSafeType(Qt, getContext()))
      return;

    if (isAwaitableType(Qt))
      return;
      
    if (isRawPointerType(Qt))
      return;

    if (auto Np = isNakedPointerType(Qt, getContext())) {
      if (Np.isOk())
        return;

      auto Dh = DiagHelper(this);
      Dh.diag(Loc, "unsafe type at temporary expression");
      isNakedPointerType(Qt, getContext(), Dh); // for report
      return;
    }

    if (auto Ns = isNakedStructType(Qt, getContext())) {
      if (Ns.isOk())
        return;

      auto Dh = DiagHelper(this);
      Dh.diag(Loc, "unsafe type at temporary expression");
      isNakedStructType(Qt, getContext(), Dh); // for report
      return;
    }

    if (isLambdaType(Qt))
      return;

//    tmp->dump();
    auto Dh = DiagHelper(this);
    Dh.diag(Loc, "unsafe type at temporary expression");
    isSafeType(Qt, getContext(), Dh);
  }
}

} // namespace checker
} // namespace nodecpp
