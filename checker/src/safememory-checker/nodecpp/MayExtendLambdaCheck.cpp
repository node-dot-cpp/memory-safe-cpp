/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- MayExtendLambdaCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MayExtendLambdaCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void MayExtendLambdaCheck::registerMatchers(MatchFinder *Finder) {

  Finder->addMatcher(
      cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyParameter(hasAttr(
                                           clang::attr::SafeMemoryMayExtend)))))
                         .bind("call"),
                     this);}


bool MayExtendLambdaCheck::checkLambda(
    const LambdaExpr *Lamb, bool IsCompositeOfThis) {

  auto Caps = Lamb->captures();
  for (auto It = Caps.begin(); It != Caps.end(); ++It) {
    switch (It->getCaptureKind()) {
    case LCK_This:
      if (IsCompositeOfThis)
        break;
      else {
        auto RecDecl = getEnclosingCXXRecordDecl(getASTContext(), Lamb);
        if(RecDecl) {
          QualType Qt = getASTContext()->getTypeDeclType(RecDecl);
          if(isDerivedFromNodeBase(Qt))
            break;
        }
      }
      diag(It->getLocation(), "(S5.7) capture of 'this' unsafe to extend scope");
      return false;
    case LCK_StarThis:
      // this is ok
      break;
    case LCK_ByCopy: {
      auto D = It->getCapturedVar();
      auto Qt = D->getType().getCanonicalType();
      if (getCheckHelper()->isHeapSafe(Qt))
        break;

      if (D->hasGlobalStorage())
        break;

      diag(It->getLocation(), "(S5.7) unsafe capture to extend scope");
      return false;
    } 
    case LCK_ByRef:
      diag(It->getLocation(), "(S5.7) unsafe capture to extend scope");
      return false;
    case LCK_VLAType:
      diag(It->getLocation(), "(S5.7) capture by array not allowed");
      return false;
    }
  }
  return true;
}


bool MayExtendLambdaCheck::tryCheckAsLambda(const Expr *Ex, bool IsCompositeOfThis) {

  if (!Ex)
    return false;

  auto E = ignoreTemporaries(Ex);

  if (auto Lamb = dyn_cast<LambdaExpr>(E)) {
    checkLambda(Lamb, IsCompositeOfThis);
    return true;
  } else if (auto Ref = dyn_cast<DeclRefExpr>(E)) {
    // diag(e->getExprLoc(), "argument is declRef");
    if (auto D = dyn_cast_or_null<VarDecl>(Ref->getDecl())) {
      auto Init = D->getInit();
      if (Init) {
        auto E2 = ignoreTemporaries(Init);
        if (auto Lamb2 = dyn_cast<LambdaExpr>(E2)) {
          if (!checkLambda(Lamb2, IsCompositeOfThis)) {
            diag(Ex->getExprLoc(), "referenced from here",
                 DiagnosticIDs::Note);
          }
          return true;
        }
      }
    }
  }

  return false;
}


/* static */
bool MayExtendLambdaCheck::isCompositeOfThis(const Expr *Ex) {
  
  // here we check the base where a method with [[may_extend_to_this]] was called
  // if the base is a composite of 'this', then 'this' can be in the captures
  // without further checks, otherwise other rules must be checked

  assert (Ex);

  auto Member = dyn_cast<MemberExpr>(Ex->IgnoreParenImpCasts());
  if (!Member) {
    return false;
  }

  auto Base = Member->getBase()->IgnoreParenImpCasts();
  if (isa<CXXThisExpr>(Base))
    return true;

  if (Member->isArrow()) {
    return false;
  }

  return isCompositeOfThis(Base);
}

void MayExtendLambdaCheck::check(const MatchFinder::MatchResult &Result) {

  auto Call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("call");

  if(isSystemLocation(getContext(), Call->getExprLoc()))
    return;

  auto Decl = Call->getMethodDecl();

  auto MayExtend = isCompositeOfThis(Call->getCallee());

  for (unsigned I = 0; I != Decl->param_size(); ++I) {
    auto P = Decl->getParamDecl(I);
    if (P->hasAttr<SafeMemoryMayExtendAttr>()) {

      auto E = Call->getArg(I);
      if (isFunctionPtr(E)) {
        continue;
      } else if (tryCheckAsLambda(E, MayExtend)) {
        continue;
      }
      // e may be null?
      diag(E->getExprLoc(),
            "(S5.7) argument not safe call declaration with attribute [[safememory::may_extend_to_this]]");
    }
  }
}

} // namespace checker
} // namespace nodecpp
