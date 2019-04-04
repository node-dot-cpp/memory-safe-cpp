/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- RecordDeclCheck.cpp - clang-tidy----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RecordDeclCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {


void RecordDeclCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(cxxRecordDecl().bind("rd"), this);
}

void RecordDeclCheck::check(const MatchFinder::MatchResult &Result) {

  auto Rd = Result.Nodes.getNodeAs<CXXRecordDecl>("rd");
  if (Rd && Rd->hasDefinition()) {

    //lambda's CXXRecordDecl is not matched here, not sure why
    // but we add the check anyway
    if (Rd->isLambda())
      return;

    auto T = Rd->getDescribedClassTemplate();

    if (T) // this is a template, don't check here
      return;

    auto K = Rd->getTemplateSpecializationKind();
    if (K == TSK_ImplicitInstantiation)
      return;

    if (Rd->hasAttr<NodeCppNakedStructAttr>()) {
      if (checkNakedStructRecord(Rd, getContext()))
        return;

      auto Dh = DiagHelper(this);
      Dh.diag(Rd->getLocation(), "unsafe naked_struct declaration");
      checkNakedStructRecord(Rd, getContext(), Dh);
    } else {
      if (isSafeRecord(Rd, getContext()))
        return;

      auto Dh = DiagHelper(this);
      Dh.diag(Rd->getLocation(), "unsafe type declaration");
      isSafeRecord(Rd, getContext(), Dh);
    }
  }
}

} // namespace checker
} // namespace nodecpp
