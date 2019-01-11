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
#include "NakedPtrhelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace nodecpp {

void RecordDeclCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(cxxRecordDecl().bind("rd"), this);
}

void RecordDeclCheck::check(const MatchFinder::MatchResult &Result) {

  auto rd = Result.Nodes.getNodeAs<CXXRecordDecl>("rd");
  if(rd && rd->hasDefinition()) {

    //lambda's CXXRecordDecl is not matched here, not sure why
    // but we add the check anyway
    if(rd->isLambda())
      return;

    auto t = rd->getDescribedClassTemplate();
    
    if(t) // this is a template, don't check here
      return;

    auto k = rd->getTemplateSpecializationKind();
    if(k == TSK_ImplicitInstantiation)
      return;

    if(rd->hasAttr<NodeCppNakedStructAttr>()) {
      if(checkNakedStructRecord(rd, getContext()))
        return;

      auto dh = DiagHelper(this);
      dh.diag(rd->getLocation(), "unsafe naked_struct declaration");
      checkNakedStructRecord(rd, getContext(), dh);
    }
    else {
      if(isSafeRecord(rd, getContext()))
        return;

      auto dh = DiagHelper(this);
      dh.diag(rd->getLocation(), "unsafe type declaration");
      isSafeRecord(rd, getContext(), dh);
    }
  }
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
