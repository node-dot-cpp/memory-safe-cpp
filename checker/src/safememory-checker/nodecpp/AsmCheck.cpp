/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- AsmCheck.cpp - clang-tidy-----------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AsmCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void AsmCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(asmStmt().bind("asm"), this);
}

void AsmCheck::check(const MatchFinder::MatchResult &Result) {

  auto St = Result.Nodes.getNodeAs<AsmStmt>("asm");

  diag(St->getAsmLoc(), "(S6.1) asm is prohibited");
}

} // namespace checker
} // namespace nodecpp
