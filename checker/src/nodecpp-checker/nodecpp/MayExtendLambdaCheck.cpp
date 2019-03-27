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
                                           clang::attr::NodeCppOwnedByThis)))))
                         .bind("bad"),
                     this);


  Finder->addMatcher(
      cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyParameter(hasAttr(
                                           clang::attr::NodeCppMayExtend)))))
                         .bind("call"),
                     this);
}

bool MayExtendLambdaCheck::checkLambda(
    const LambdaExpr *Lamb, std::pair<bool, const ValueDecl *> Decl) {

  auto Caps = Lamb->captures();
  for (auto It = Caps.begin(); It != Caps.end(); ++It) {
    switch (It->getCaptureKind()) {
    case LCK_This:
      if (Decl.first)
        break;

      diag(It->getLocation(), "capture of 'this' unsafe to extend scope");
      return false;
    case LCK_StarThis:
      // this is ok
      break;
    case LCK_ByCopy: {
      auto D = It->getCapturedVar();
      auto Qt = D->getType().getCanonicalType();
      if (isSafeType(Qt, getContext()))
        break;

      if (D->hasGlobalStorage())
        break;

      if (D->hasAttr<NodeCppMayExtendAttr>())
        break;

      if (D == Decl.second) // ok
        break;

      diag(It->getLocation(), "unsafe capture to extend scope");
    } return false;
    case LCK_ByRef:
      diag(It->getLocation(), "unsafe capture to extend scope");
      return false;
    case LCK_VLAType:
      diag(It->getLocation(), "capture by array not allowed");
      return false;
    default:
      diag(It->getLocation(), "unknow capture kind not allowed");
      return false;
    }
  }
  return true;
}

// void MayExtendLambdaCheck::checkLambda2(const LambdaExpr *lamb) {

//   auto fields = lamb->getLambdaClass()->fields();
//   for (auto it = fields.begin(); it != fields.end(); ++it) {
//     if (isSafeType(it->getType()))
//       continue;

//     auto ptr = it->getType().getCanonicalType().getTypePtrOrNull();
//     if (!ptr)
//       continue; // some type error, not our problem

//     if (!ptr->isPointerType()) {
//       diag(it->getLocation(), "capture type not allowed");
//       continue;
//     }
//     auto r = ptr->getPointeeCXXRecordDecl();
//     if (!r->hasAttr<NodeCppNoInstanceAttr>()) {
//       diag(it->getLocation(), "capture pointer type not allowed");
//       continue;
//     }

//     // we are safe continue with next
//   }
// }

/* static */
// const LambdaExpr *MayExtendLambdaCheck::getLambda(const Expr *expr) {

//   if (!expr)
//     return nullptr;

//   auto e = ignoreTemporaries(expr);

//   if (auto lamb = dyn_cast<LambdaExpr>(e)) {
//     return lamb;
//   } else if (auto ref = dyn_cast<DeclRefExpr>(e)) {
//     // diag(e->getExprLoc(), "argument is declRef");
//     if (auto d = dyn_cast_or_null<VarDecl>(ref->getDecl())) {
//       return getLambda(d->getInit());
//     }
//   }

//   return nullptr;
// }

//returns true if the expression was checked (either positively or negatively)
bool MayExtendLambdaCheck::tryCheckAsLambda(
    const Expr *Ex, std::pair<bool, const ValueDecl *> Decl) {
  // const LambdaExpr *MayExtendLambdaCheck::getLambda(const Expr *expr) {

  if (!Ex)
    return false;

  auto E = ignoreTemporaries(Ex);

  if (auto Lamb = dyn_cast<LambdaExpr>(E)) {
    checkLambda(Lamb, Decl);
    return true;
  } else if (auto Ref = dyn_cast<DeclRefExpr>(E)) {
    // diag(e->getExprLoc(), "argument is declRef");
    if (auto D = dyn_cast_or_null<VarDecl>(Ref->getDecl())) {
      auto Init = D->getInit();
      if (Init) {
        auto E2 = ignoreTemporaries(Init);
        if (auto Lamb2 = dyn_cast<LambdaExpr>(E2)) {
          if (!checkLambda(Lamb2, Decl)) {
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
bool MayExtendLambdaCheck::canMayExtendBeCalled(const Expr *Ex) {
  // here we allow:

  // this->call()

  // this->member.call()
  // this->member.member.call()

  // [[may_extend]]->call()
  // [[may_extend]]->member.call()

  // [[owned_by_this]]->call()
  // [[owned_by_this]]->member.call()

  if (!Ex) {
    assert(false);
    return false;
  }

  auto Member = dyn_cast<MemberExpr>(Ex->IgnoreParenImpCasts());
  if (!Member) {
    return false;
  }

  auto Base = Member->getBase()->IgnoreParenImpCasts();
  if (isa<CXXThisExpr>(Base))
    return true;
  else if (auto Var = dyn_cast<DeclRefExpr>(Base)) {
    auto Decl = Var->getDecl();
    if (!Decl) { // shouln't happend here
      return false;
    }

    if (Decl->hasAttr<NodeCppMayExtendAttr>())
      return true;
    else if (Decl->hasAttr<NodeCppOwnedByThisAttr>())
      return true;
    else
      return false;
  } else if (auto OsnBase = getBaseIfOsnPtrDerref(Base)) {
    if (auto Var = dyn_cast<DeclRefExpr>(OsnBase->IgnoreParenImpCasts())) {
      auto Decl = Var->getDecl();
      if (!Decl) { // shouln't happend here
        return false;
      }

      if (Decl->hasAttr<NodeCppMayExtendAttr>())
        return true;
      else if (Decl->hasAttr<NodeCppOwnedByThisAttr>())
        return true;
      else
        return true;
    }
  }

  if (Member->isArrow()) {
    return false;
  }

  return canMayExtendBeCalled(Base);
}

/* static */
std::pair<bool, const ValueDecl *>
MayExtendLambdaCheck::canMayExtendBeCalled2(const Expr *Ex) {
  // here we check the base where a method with [[may_extend_to_this]] was called
  // if the base has the lifetime of 'this', then 'this' or any other member
  // with the lifetime of 'this' may be an argument

  // if the base has other lifetime then only the base itself can be the argument

  // this->call()

  // this->member.call()
  // this->member.member.call()

  // [[may_extend]]->call()
  // [[may_extend]]->member.call()

  // [[owned_by_this]]->call()
  // [[owned_by_this]]->member.call()

  if (!Ex) {
    assert(false);
    return {false, nullptr};
  }

  auto Member = dyn_cast<MemberExpr>(Ex->IgnoreParenImpCasts());
  if (!Member) {
    return {false, nullptr};
  }

  auto Base = Member->getBase()->IgnoreParenImpCasts();
  if (isa<CXXThisExpr>(Base))
    return {true, nullptr};
  else if (auto Var = dyn_cast<DeclRefExpr>(Base)) {
    auto Decl = Var->getDecl();
    if (!Decl) { // shouln't happend here
      return {false, nullptr};
    }

    if (Decl->hasAttr<NodeCppMayExtendAttr>())
      return {true, Decl};
    else if (Decl->hasAttr<NodeCppOwnedByThisAttr>())
      return {true, Decl};
    else
      return {false, Decl};
  } else if (auto OsnBase = getBaseIfOsnPtrDerref(Base)) {
    if (auto Var = dyn_cast<DeclRefExpr>(OsnBase->IgnoreParenImpCasts())) {
      auto Decl = Var->getDecl();
      if (!Decl) { // shouln't happend here
        return {false, nullptr};
      }

      if (Decl->hasAttr<NodeCppMayExtendAttr>())
        return {true, Decl};
      else if (Decl->hasAttr<NodeCppOwnedByThisAttr>())
        return {true, Decl};
      else
        return {false, Decl};
    }
  }

  if (Member->isArrow()) {
    return {false, nullptr};
  }

  return canMayExtendBeCalled2(Base);
}

void MayExtendLambdaCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto Bad = Result.Nodes.getNodeAs<CXXMemberCallExpr>("bad")) {
    diag(Bad->getExprLoc(), "calling method with [[owned_by_this]] attribute "
                            "from safe code is not implemented yet");
    return;
  }

  auto Call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
  auto Decl = Call->getMethodDecl();

  auto Callee = Call->getCallee();
  auto MayExtend = canMayExtendBeCalled2(Callee);
  // if(!mayExtend.first) {
  //   diag(callee->getExprLoc(), "methods with attribute can be called only on members that share the lifetime of this, or have attribute themselves");
  //   return;
  // }

  for (unsigned I = 0; I != Decl->param_size(); ++I) {
    auto P = Decl->getParamDecl(I);
    if (P->hasAttr<NodeCppMayExtendAttr>()) {

      auto E = Call->getArg(I);
      if (isFunctionPtr(E)) {
        continue;
      } else if (tryCheckAsLambda(E, MayExtend)) {
        continue;
      } else {
        if (MayExtend.first) {
          auto Ch =
              NakedPtrScopeChecker::makeThisScopeChecker(this, getContext());
          if (Ch.checkExpr(E))
            continue;
        }
        if (auto Dr = dyn_cast<DeclRefExpr>(ignoreTemporaries(E))) {
          if (Dr->getDecl() == MayExtend.second)
            continue;
        }
      }
            // e may be null?
            diag(E->getExprLoc(),
                 "is not safe to extend argument scope to 'this'");
    }
  }
}

} // namespace checker
} // namespace nodecpp
