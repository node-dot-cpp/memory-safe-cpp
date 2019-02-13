/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- PtrArithmeticCheck.cpp - clang-tidy-------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PtrArithmeticCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace nodecpp {

void PtrArithmeticCheck::registerMatchers(MatchFinder *Finder) {
}

void PtrArithmeticCheck::check(const MatchFinder::MatchResult &Result) {

}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
