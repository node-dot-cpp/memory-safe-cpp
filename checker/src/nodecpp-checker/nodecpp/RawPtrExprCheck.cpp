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

namespace nodecpp {
namespace checker {

void RawPtrExprCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(expr(hasType(pointerType())).bind("expr"),
      this);

  Finder->addMatcher(expr(hasType(referenceType())).bind("expr"),
      this);
}

void RawPtrExprCheck::check(const MatchFinder::MatchResult &Result) {
  auto Ex = Result.Nodes.getNodeAs<Expr>("expr");

  //first ignore implicits and parens

  if (isImplicitExpr(Ex))
    return;
  else if (isa<CXXDefaultArgExpr>(Ex))
    return;

  // now allow some explicits
  else if (isa<CXXThisExpr>(Ex))
    return;
  else if (isa<CallExpr>(Ex))
    return;
  else if (isa<CXXNullPtrLiteralExpr>(Ex))
    return;
  else if (isa<CXXDynamicCastExpr>(Ex))
    return;

  else if (auto UnOp = dyn_cast<UnaryOperator>(Ex)) {
    if (UnOp->getOpcode() == UnaryOperatorKind::UO_AddrOf) {
      //this is ok
      return;
    }
  }

  else if (auto BinOp = dyn_cast<BinaryOperator>(Ex)) {
    if (BinOp->getOpcode() == BinaryOperatorKind::BO_Assign) {
      //this is ok
      return;
    }
  }

  // this is an error,
  // find the best error message
  else if (isa<DeclRefExpr>(Ex)) {
    // don't report it here
    return;
  } else if (isa<CXXNewExpr>(Ex)) {
    // don't report here
    return;
  } else if (isa<CXXDeleteExpr>(Ex)) {
    // don't report here
    return;
  } else if (isa<CXXConstCastExpr>(Ex)) {
    // don't report here
    return;
  }

  else if (isa<CXXStaticCastExpr>(Ex)) {
    diag(Ex->getExprLoc(), "(S1.1) static_cast not allowed");
    return;
  }

  else if (isa<CXXReinterpretCastExpr>(Ex)) {
    diag(Ex->getExprLoc(), "(S1.1) reinterpret_cast not allowed");
    return;
  }

  else if (isa<CStyleCastExpr>(Ex)) {
    diag(Ex->getExprLoc(), "(S1.1) C style cast not allowed");
    return;
  }

  diag(Ex->getExprLoc(), "(S1) raw pointer expression not allowed");
}

} // namespace checker
} // namespace nodecpp
