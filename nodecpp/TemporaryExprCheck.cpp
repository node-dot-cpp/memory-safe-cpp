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

namespace clang {
namespace tidy {
namespace nodecpp {

void TemporaryExprCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(cxxTemporaryObjectExpr().bind("tmp"), this);
}

void TemporaryExprCheck::check(const MatchFinder::MatchResult &Result) {
 
  if(auto tmp = Result.Nodes.getNodeAs<CXXTemporaryObjectExpr>("tmp")) {

    QualType qt = tmp->getType().getCanonicalType();
    if (isSafeType(qt, getContext()))
      return;
    if(isAnyFunctionType(qt))
      return;

    if(isRawPointerType(qt))
      return;
    
    if(auto np = isNakedPointerType(qt, getContext())) {
      if(np.isOk())
        return;

      auto dh = DiagHelper(this);
      dh.diag(tmp->getExprLoc(), "unsafe type at temporary expression");
      isNakedPointerType(qt, getContext(), dh); //for report
      return;
    }
    
    if(auto ns = isNakedStructType(qt, getContext())) {
      if(ns.isOk())
        return;

      auto dh = DiagHelper(this);
      dh.diag(tmp->getExprLoc(), "unsafe type at temporary expression");
      isNakedStructType(qt, getContext(), dh); //for report
      return;
    }

    if(isLambdaType(qt))
      return;

//    tmp->dump();
    auto dh = DiagHelper(this);
    dh.diag(tmp->getExprLoc(), "unsafe type at temporary expression");
    isSafeType(qt, getContext(), dh);
  }
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
