/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- NewExprCheck.cpp - clang-tidy-------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NakedPtrHelper.h"
#include "NewExprCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void NewExprCheck::registerMatchers(MatchFinder *Finder) {

  if (!getLangOpts().CPlusPlus)
    return;

  Finder->addMatcher(
      cxxNewExpr().bind("new"), this);
  Finder->addMatcher(
      cxxDeleteExpr().bind("delete"), this);

  Finder->addMatcher(
      callExpr(callee(functionDecl(hasName("nodecpp::safememory::make_owning"))))
          .bind("make"),
      this);
}

bool NewExprCheck::checkParentExpr(ASTContext *context, const Expr *expr) {

  auto sList = context->getParents(*expr);

  auto sIt = sList.begin();

  if (sIt == sList.end())
    return false;

  if(auto p = sIt->get<Expr>()) {
    if (isa<ParenExpr>(p) || isa<ImplicitCastExpr>(p) ||
      isa<CXXBindTemporaryExpr>(p) || isa<ExprWithCleanups>(p) ||
      isa<CXXConstructExpr>(p)) {
      
      return checkParentExpr(context, p);
    }
    else if(auto tmp = dyn_cast<MaterializeTemporaryExpr>(p)) {
      if(tmp->isXValue()) {
        return checkParentExpr(context, p);
      }
      else
        return false;
    }
    else if(isa<CallExpr>(p)) {
      return true;
    }
  }
  else if(auto p = sIt->get<VarDecl>()) {
    return true;
  }

  return false;
}


void NewExprCheck::check(const MatchFinder::MatchResult &Result) {

  if(auto expr = Result.Nodes.getNodeAs<CXXNewExpr>("new")) {

    diag(expr->getExprLoc(), "(S4) operator new is prohibited");
  }
  else if(auto expr = Result.Nodes.getNodeAs<CXXDeleteExpr>("delete")) {

    diag(expr->getExprLoc(), "(S4) operator delete is prohibited");
  }
  else if(auto expr = Result.Nodes.getNodeAs<CallExpr>("make")) {

    if(!checkParentExpr(Result.Context, expr)) {
      diag(expr->getExprLoc(), "(S4.1) result of make_owning must be assigned to owning_ptr");
    }
  }
}

} // namespace checker
} // namespace nodecpp
