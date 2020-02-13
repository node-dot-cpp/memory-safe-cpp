/*******************************************************************************
* Copyright (C) 2019 OLogN Technologies AG
* All rights reserved.
*******************************************************************************/
//===--- AwaitableCheck.cpp - clang-tidy-----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AwaitableCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void AwaitableCheck::registerMatchers(MatchFinder *Finder) {
 
  Finder->addMatcher(expr().bind("expr"), this);
}

void AwaitableCheck::check(const MatchFinder::MatchResult &Result) {

  auto Ex = Result.Nodes.getNodeAs<Expr>("expr");

  return;

  //some implicit expr don't even have a type
  if(isImplicitExpr(Ex))
    return;

  QualType Qt = Ex->getType();
  //some late parsing template constructs don't have type
  if(Qt.isNull())
    return;

  if(!isAwaitableType(Qt))
    return;

  auto Pex = getParentExpr(Result.Context, Ex);

  if(Pex) {
    if(isa<CoawaitExpr>(Pex))
      return;
    else if(auto Mex = dyn_cast<MemberExpr>(Pex)) {
      if(Mex->getMemberDecl()->getNameAsString() == "await_ready")
        return;
    }
    else if(isa<DeclRefExpr>(Pex))
      return; //The error goes to the declaration initializer, not here
    else if(auto Cex = dyn_cast<CallExpr>(Pex) ) {
      auto Fdecl = Cex->getDirectCallee();
      if(Fdecl) {
        std::string Name = getQnameForSystemSafeDb(Fdecl);
        if(isWaitForAllName(Name))
          return;
        else if(Fdecl->hasAttr<NodeCppNoAwaitAttr>())
          return;
      }
    }
  }

  diag(Ex->getExprLoc(), "(S9.1) awaitable expression must be used with co_await");

//      << FixItHint::CreateInsertion(MatchedDecl->getLocation(), "awesome_");
}

} // namespace checker
} // namespace nodecpp
