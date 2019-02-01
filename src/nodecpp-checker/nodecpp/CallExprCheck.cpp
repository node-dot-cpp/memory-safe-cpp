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

namespace clang {
namespace tidy {
namespace nodecpp {

void CallExprCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(callExpr().bind("call"), this);
}

void CallExprCheck::check(const MatchFinder::MatchResult &Result) {

  auto expr = Result.Nodes.getNodeAs<CallExpr>("call");

  auto decl = expr->getDirectCallee();
  if(!decl)
    return;

  if(isa<CXXMethodDecl>(decl))
    return;

  SourceManager* manager = Result.SourceManager;
  auto eLoc = manager->getExpansionLoc(decl->getLocStart());

  if(eLoc.isInvalid())
    return;

  if(!manager->isInSystemHeader(eLoc))
    return; // this is in safe code, then is ok
 

  std::string name = decl->getQualifiedNameAsString();
  auto& s = getContext()->getGlobalOptions().SafeFunctions;
  if(s.find(name) != s.end())
    return;

  diag(expr->getExprLoc(), "(S8) unsafe function call is prohibited");
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
