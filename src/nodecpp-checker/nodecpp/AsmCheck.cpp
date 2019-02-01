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

namespace clang {
namespace tidy {
namespace nodecpp {

void AsmCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(asmStmt().bind("asm"), this);
}

void AsmCheck::check(const MatchFinder::MatchResult &Result) {


  auto stmt = Result.Nodes.getNodeAs<AsmStmt>("asm");
  
  diag(stmt->getAsmLoc(), "(S6.1) asm is prohibited");
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
