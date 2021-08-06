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

  if(isSystemLocation(getContext(), Loc))
    return;


  if (!Qt.isNull()) {

    Qt = Qt.getCanonicalType();
    if (isStdFunctionType(Qt))
      return;

    if(Qt->isConstantArrayType()) {
      //we allow constant array type at temps for std::initializer_list
      if(auto ArrT = Qt->getAsArrayTypeUnsafe()) {
        Qt = ArrT->getElementType().getCanonicalType();
      }
    }

    if (getCheckHelper()->isHeapSafe(Qt))
      return;

    if (isAwaitableType(Qt))
      return;

    if (getCheckHelper()->isStackOnlyIterator(Qt))
      return;

    if (isRawPointerType(Qt))
      return;

    if (auto Np = getCheckHelper()->checkNullablePtr(Qt)) {
      if (Np.isOk())
        return;

      getContext()->diagError2(Loc, "nullable-ptr", "unsafe type at temporary expression");
      getCheckHelper()->reportNullablePtrDetail(Qt); // for report
      return;
    }

    if (auto Ns = getCheckHelper()->checkNakedStruct(Qt)) {
      if (Ns.isOk())
        return;

      getContext()->diagError2(Loc, "naked-struct", "unsafe type at temporary expression");
      getCheckHelper()->reportNakedStructDetail(Qt); // for report
      return;
    }

    if (isLambdaType(Qt))
      return;

//    tmp->dump();
    getContext()->diagError2(Loc, "heap-safe", "unsafe type at temporary expression");
    getCheckHelper()->reportNonSafeDetail(Qt);
  }
}

} // namespace checker
} // namespace nodecpp
