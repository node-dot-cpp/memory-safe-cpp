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
      callExpr(callee(functionDecl(hasName("safememory::make_owning"))))
          .bind("make"),
      this);
  Finder->addMatcher(
      callExpr(callee(functionDecl(hasName("safememory::make_owning_2"))))
          .bind("make"),
      this);
}

bool NewExprCheck::checkParentExpr(ASTContext *Context, const Expr *Ex) {

  auto SList = Context->getParents(*Ex);

  auto SIt = SList.begin();

  if (SIt == SList.end())
    return false;

  if (auto P = SIt->get<Expr>()) {
    if (isa<ParenExpr>(P) || isa<ImplicitCastExpr>(P) ||
        isa<CXXBindTemporaryExpr>(P) || isa<ExprWithCleanups>(P) ||
        isa<CXXConstructExpr>(P)) {

      return checkParentExpr(Context, P);
    } else if (auto Tmp = dyn_cast<MaterializeTemporaryExpr>(P)) {
      if (Tmp->isXValue()) {
        return checkParentExpr(Context, P);
      } else
        return false;
    } else if (isa<CallExpr>(P)) {
      return true;
    }
  } else if (SIt->get<VarDecl>()) {
    return true;
  }

  return false;
}

void NewExprCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto Expr = Result.Nodes.getNodeAs<CXXNewExpr>("new")) {

    if(isSystemLocation(getContext(), Expr->getExprLoc()))
      return;

    diag(Expr->getExprLoc(), "(S4) operator new is prohibited");
  } else if (auto Expr = Result.Nodes.getNodeAs<CXXDeleteExpr>("delete")) {

    if(isSystemLocation(getContext(), Expr->getExprLoc()))
      return;

    diag(Expr->getExprLoc(), "(S4) operator delete is prohibited");
  } else if (auto Expr = Result.Nodes.getNodeAs<CallExpr>("make")) {

    if(isSystemLocation(getContext(), Expr->getExprLoc()))
      return;

    QualType Rt = Expr->getCallReturnType(*getASTContext());
    
    Rt = Rt.getCanonicalType();
    assert(isSafePtrType(Rt));

    auto Pointee = getPointeeType(Rt);
    if(Pointee && isDerivedFromNodeBase(*Pointee)) {
      diag(Expr->getExprLoc(),
           "(node-dot-cpp) clases derived from NodeBase can't be instantiated by user");

      return;
    }

    if (!checkParentExpr(Result.Context, Expr)) {
      diag(Expr->getExprLoc(),
           "(S4.1) result of make_owning must be assigned to owning_ptr");
    }
  }
}

} // namespace checker
} // namespace nodecpp
