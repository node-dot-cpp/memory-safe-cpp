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

    //only check the definition decl
    if(Rd != Rd->getDefinition())
      return;

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

    QualType Qt = getASTContext()->getRecordType(Rd);

    if(auto Ns = getCheckHelper()->checkNakedStruct(Qt)) {
      if(Ns.isOk())
        return;

      getContext()->diagError(Rd->getLocation(), "xxx", "unsafe naked_struct declaration");
      getCheckHelper()->reportNakedStructDetail(Qt);
    }
    else if(auto Dc = getCheckHelper()->checkDeepConst(Qt)) {
      if(Dc.isOk())
        return;

      getContext()->diagError(Rd->getLocation(), "xxx", "unsafe [[deep_const]] declaration");
      getCheckHelper()->reportDeepConstDetail(Qt);
    }
    else {
      if (getCheckHelper()->isHeapSafe(Qt))
        return;

      getContext()->diagError(Rd->getLocation(), "xxx", "unsafe type declaration");
      getCheckHelper()->reportNonSafeDetail(Qt);
    }
  }
}

} // namespace checker
} // namespace nodecpp
