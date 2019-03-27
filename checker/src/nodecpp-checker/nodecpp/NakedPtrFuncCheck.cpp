/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- NakedPtrFuncCheck.cpp - clang-tidy--------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NakedPtrFuncCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void NakedPtrFuncCheck::registerMatchers(MatchFinder *Finder) {

	// Will match implicit operator new and delete, ignore implicits
  // just pointer type case is handled by naked-ptr-from-return
  Finder->addMatcher(functionDecl(unless(isImplicit()),
                                  returns(pointerType(pointee(pointerType()))))
                         .bind("func"),
                     this);
  Finder->addMatcher(
      functionDecl(unless(isImplicit()),
                   returns(referenceType(pointee(pointerType()))))
          .bind("func"),
      this);

  Finder->addMatcher(functionDecl(unless(isImplicit()),
                                  hasAnyParameter(hasType(
                                      referenceType(pointee(pointerType())))))
                         .bind("func"),
                     this);
  Finder->addMatcher(functionDecl(unless(isImplicit()),
                                  hasAnyParameter(hasType(
                                      pointerType(pointee(pointerType())))))
                         .bind("func"),
                     this);
}

void NakedPtrFuncCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *Func = Result.Nodes.getNodeAs<FunctionDecl>("func");
  // This matchers matches implicit operator new and delete without location,
  // ignore them
  if (!Func->getLocation().isValid()) {
    Func->dumpColor();
    return;
  }
  auto T = Func->getReturnType();
  if (T->isReferenceType() || T->isPointerType()) {
    auto Pt = T->getPointeeType();
    if (Pt->isPointerType()) {
      diag(Func->getLocation(), "return type not allowed");
      return;
    }
  }

  auto Args = Func->parameters();
  for (auto It = Args.begin(); It != Args.end(); ++It) {
    auto T = (*It)->getType();
    if (T->isPointerType() || T->isReferenceType()) {
      auto Pt = T->getPointeeType();
      if (Pt->isPointerType()) {
        diag((*It)->getLocation(), "parameter type not allowed");
        return;
      }
    }
  }
  diag(Func->getLocation(),
       "there is an unknown problem with the signature of this function");
  Func->dumpColor();
}

} // namespace checker
} // namespace nodecpp
