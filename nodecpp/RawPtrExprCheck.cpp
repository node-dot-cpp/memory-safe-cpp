/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- RawPtrExprCheck.cpp - clang-tidy----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RawPtrExprCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace nodecpp {

void RawPtrExprCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(expr(hasType(pointerType())).bind("expr"),
      this);

  Finder->addMatcher(expr(hasType(referenceType())).bind("expr"),
      this);
}

void RawPtrExprCheck::check(const MatchFinder::MatchResult &Result) {
  auto expr = Result.Nodes.getNodeAs<Expr>("expr");

  //first ignore implicits and parens

  if(isa<ExprWithCleanups>(expr))
    return;
  else if(isa<MaterializeTemporaryExpr>(expr))
    return;
  else if(isa<CXXBindTemporaryExpr>(expr))
    return;
  else if(isa<ImplicitCastExpr>(expr))
    return;
  else if(isa<ParenExpr>(expr))
    return;
  else if(isa<CXXDefaultArgExpr>(expr))
    return;

  // now allow some explicits
  else if(isa<CXXThisExpr>(expr))
    return;
  else if(isa<CallExpr>(expr))
    return;
  else if(isa<CXXNullPtrLiteralExpr>(expr))
    return;
  else if(isa<CXXDynamicCastExpr>(expr))
    return;

  else if(auto unOp = dyn_cast<UnaryOperator>(expr)) {
    if(unOp->getOpcode() == UnaryOperatorKind::UO_AddrOf) {
      //this is ok
      return;
    }
  }

  else if(auto binOp = dyn_cast<BinaryOperator>(expr)) {
    if(binOp->getOpcode() == BinaryOperatorKind::BO_Assign) {
      //this is ok
      return;
    }
  }

  // this is an error,
  // find the best error message
  else if(isa<DeclRefExpr>(expr)) {
      // don't report it here
      return;
  }
  else if(isa<CXXNewExpr>(expr)) {
      // don't report here
      return;
  }
  else if(isa<CXXDeleteExpr>(expr)) {
      // don't report here
      return;
  }

  else if(isa<CXXStaticCastExpr>(expr)) {
    diag(expr->getExprLoc(), "(S1.1) static_cast not allowed");
    return;
  }

  else if(isa<CXXReinterpretCastExpr>(expr)) {
    diag(expr->getExprLoc(), "(S1.1) reinterpret_cast not allowed");
    return;
  }

  else if(isa<CStyleCastExpr>(expr)) {
    diag(expr->getExprLoc(), "(S1.1) C style cast not allowed");
    return;
  }

  diag(expr->getExprLoc(), "(S1) raw pointer expression not allowed");
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
