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

namespace clang {
namespace tidy {
namespace nodecpp {

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

  const auto *func = Result.Nodes.getNodeAs<FunctionDecl>("func");
  // This matchers matches implicit operator new and delete without location,
  // ignore them
  if (!func->getLocation().isValid()) {
    func->dumpColor();
    return;
  }
  auto t = func->getReturnType();
  if (t->isReferenceType() || t->isPointerType()) {
    auto pt = t->getPointeeType();
    if (pt->isPointerType()) {
      diag(func->getLocation(), "return type not allowed");
      return;
    }
  }

  auto args = func->parameters();
  for (auto it = args.begin(); it != args.end(); ++it) {
    auto t = (*it)->getType();
    if (t->isPointerType() || t->isReferenceType()) {
      auto pt = t->getPointeeType();
      if (pt->isPointerType()) {
        diag((*it)->getLocation(), "parameter type not allowed");
        return;
	  }
	}
  }
  diag(func->getLocation(), "there is an unknown problem with the signature of this function");
  func->dumpColor();
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
