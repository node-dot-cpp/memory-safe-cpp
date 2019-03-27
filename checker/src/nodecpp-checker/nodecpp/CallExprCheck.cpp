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

  if (isa<CXXMethodDecl>(Decl))
    return;

  SourceManager *Manager = Result.SourceManager;
  auto ELoc = Manager->getExpansionLoc(Decl->getLocStart());

  if (ELoc.isInvalid())
    return;

  if (!Manager->isInSystemHeader(ELoc))
    return; // this is in safe code, then is ok

  std::string Name = Decl->getQualifiedNameAsString();
  auto &S = getContext()->getGlobalOptions().SafeFunctions;
  if (S.find(Name) != S.end())
    return;

  diag(Ex->getExprLoc(),
       "(S8) unsafe function call '" + Name + "' is prohibited");
}

} // namespace checker
} // namespace nodecpp
