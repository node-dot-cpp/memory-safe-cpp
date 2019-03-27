/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- StaticStorageCheck.cpp - clang-tidy-------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "StaticStorageCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void StaticStorageCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(varDecl(hasStaticStorageDuration(), unless(isConstexpr())).bind("decl"), this);
  Finder->addMatcher(varDecl(hasThreadStorageDuration()).bind("decl"), this);
}

void StaticStorageCheck::check(const MatchFinder::MatchResult &Result) {

  auto Decl = Result.Nodes.getNodeAs<VarDecl>("decl");
  if (Decl->isConstexpr())
    return;

  auto Qt = Decl->getType().getCanonicalType();
  if (Qt.isConstQualified() && Qt->isBuiltinType())
    return;

  diag(Decl->getLocation(),
       "(S3) global, static or thread_local variables are prohibited");
  //<< MatchedDecl
  //<< FixItHint::CreateInsertion(MatchedDecl->getLocation(), "awesome_");
}

} // namespace checker
} // namespace nodecpp
