/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- CallExprCheck.cpp - clang-tidy------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CallExprCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void CallExprCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(callExpr().bind("call"), this);
}

void CallExprCheck::check(const MatchFinder::MatchResult &Result) {

  auto Ex = Result.Nodes.getNodeAs<CallExpr>("call");

  auto Decl = Ex->getDirectCallee();
  if (!Decl)
    return;

  SourceManager *Manager = Result.SourceManager;
  auto ELoc = Manager->getExpansionLoc(Decl->getLocation());

  if(!isSystemLocation(getContext(), ELoc))
    return; // this is in user code, then is ok

  std::string Name = getQnameForSystemSafeDb(Decl);

  if(isSoftPtrCastName(Name)) {
    diag(Ex->getExprLoc(), "(S1.1.1) soft_ptr cast is prohibited in safe code");
    return;
  }

  if(getContext()->getGlobalOptions().DisableLibraryDb)
    return;

  if(isSystemSafeFunction(Decl, getContext()))
    return;

  diag(Ex->getExprLoc(),
       "(S8) function call '" + Name + "' is not listed as safe, therefore is prohibited");
}

} // namespace checker
} // namespace nodecpp
