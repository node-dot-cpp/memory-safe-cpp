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

namespace clang {
namespace tidy {
namespace nodecpp {

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

bool MayExtendLambdaCheck::checkLambda(const LambdaExpr *lamb, std::pair<bool, const ValueDecl*> decl) {
  
  auto caps = lamb->captures();
  for (auto it = caps.begin(); it != caps.end(); ++it) {
    switch (it->getCaptureKind()) {
    case LCK_This:
      if(decl.first)
        break;
      
      diag(it->getLocation(), "capture of 'this' unsafe to extend scope");
      return false;
    case LCK_StarThis:
      // this is ok
      break;
    case LCK_ByCopy: {
      auto d = it->getCapturedVar();
      auto qt = d->getType().getCanonicalType();
      if (isSafeType(qt, getContext()))
        break;

      if (d->hasGlobalStorage())
        break;

      if(d->hasAttr<NodeCppMayExtendAttr>())
        break;

      if(d == decl.second) //ok
        break;

      diag(it->getLocation(), "unsafe capture to extend scope");
    } return false;
    case LCK_ByRef:
      diag(it->getLocation(), "unsafe capture to extend scope");
      return false;
    case LCK_VLAType:
      diag(it->getLocation(), "capture by array not allowed");
      return false;
    default:
      diag(it->getLocation(), "unknow capture kind not allowed");
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
bool MayExtendLambdaCheck::tryCheckAsLambda(const Expr *expr, std::pair<bool, const ValueDecl*> decl) {
//const LambdaExpr *MayExtendLambdaCheck::getLambda(const Expr *expr) {

  if (!expr)
    return false;

  auto e = ignoreTemporaries(expr);

  if (auto lamb = dyn_cast<LambdaExpr>(e)) {
    checkLambda(lamb, decl);
    return true;
  } else if (auto ref = dyn_cast<DeclRefExpr>(e)) {
    // diag(e->getExprLoc(), "argument is declRef");
    if (auto d = dyn_cast_or_null<VarDecl>(ref->getDecl())) {
      auto init = d->getInit();
      if(init) {
        auto e2 = ignoreTemporaries(init);
        if (auto lamb2 = dyn_cast<LambdaExpr>(e2)) {
          if(!checkLambda(lamb2, decl)) {
            diag(expr->getExprLoc(), "referenced from here", DiagnosticIDs::Note);
          }
          return true;
        }
      }       
    }
  }

  return false;
}



/* static */
bool MayExtendLambdaCheck::canMayExtendBeCalled(const Expr* expr) {
  // here we allow:

  // this->call()

  // this->member.call()
  // this->member.member.call()

  // [[may_extend]]->call()
  // [[may_extend]]->member.call()

  // [[owned_by_this]]->call()
  // [[owned_by_this]]->member.call()


  if(!expr) {
    assert(false);
    return false;
  }

  auto member = dyn_cast<MemberExpr>(expr->IgnoreParenImpCasts());
  if(!member) {
    return false;
  }

  auto base = member->getBase()->IgnoreParenImpCasts();
  if(isa<CXXThisExpr>(base))
    return true;
  else if(auto var = dyn_cast<DeclRefExpr>(base)) {
    auto decl = var->getDecl();
    if (!decl) { // shouln't happend here
      return false;
    }

    if(decl->hasAttr<NodeCppMayExtendAttr>())
      return true;
    else if(decl->hasAttr<NodeCppOwnedByThisAttr>())
      return true;
    else
      return false;
  } else if(auto osnBase = getBaseIfOsnPtrDerref(base)) {
    if(auto var = dyn_cast<DeclRefExpr>(osnBase->IgnoreParenImpCasts())) {
      auto decl = var->getDecl();
      if (!decl) { // shouln't happend here
        return false;
      }

      if(decl->hasAttr<NodeCppMayExtendAttr>())
        return true;
      else if(decl->hasAttr<NodeCppOwnedByThisAttr>())
        return true;
      else
        return true;
    }
  }


  if(member->isArrow()) {
    return false;
  }
      
  return canMayExtendBeCalled(base);
}

/* static */
std::pair<bool, const ValueDecl*> MayExtendLambdaCheck::canMayExtendBeCalled2(const Expr* expr) {
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


  if(!expr) {
    assert(false);
    return {false, nullptr};
  }

  auto member = dyn_cast<MemberExpr>(expr->IgnoreParenImpCasts());
  if(!member) {
    return {false, nullptr};
  }

  auto base = member->getBase()->IgnoreParenImpCasts();
  if(isa<CXXThisExpr>(base))
    return {true, nullptr};
  else if(auto var = dyn_cast<DeclRefExpr>(base)) {
    auto decl = var->getDecl();
    if (!decl) { // shouln't happend here
      return {false, nullptr};
    }

    if(decl->hasAttr<NodeCppMayExtendAttr>())
      return {true, decl};
    else if(decl->hasAttr<NodeCppOwnedByThisAttr>())
      return {true, decl};
    else
      return {false, decl};
  } else if(auto osnBase = getBaseIfOsnPtrDerref(base)) {
    if(auto var = dyn_cast<DeclRefExpr>(osnBase->IgnoreParenImpCasts())) {
      auto decl = var->getDecl();
      if (!decl) { // shouln't happend here
        return {false, nullptr};
      }

      if(decl->hasAttr<NodeCppMayExtendAttr>())
        return {true, decl};
      else if(decl->hasAttr<NodeCppOwnedByThisAttr>())
        return {true, decl};
      else
        return {false, decl};
    }
  }


  if(member->isArrow()) {
    return {false, nullptr};
  }
      
  return canMayExtendBeCalled2(base);
}


void MayExtendLambdaCheck::check(const MatchFinder::MatchResult &Result) {

  if(auto bad = Result.Nodes.getNodeAs<CXXMemberCallExpr>("bad")) {
    diag(bad->getExprLoc(), "calling method with [[owned_by_this]] attribute from safe code is not implemented yet");
    return;
  }

  auto call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
  auto decl = call->getMethodDecl();

  auto callee = call->getCallee();
  auto mayExtend = canMayExtendBeCalled2(callee);
  // if(!mayExtend.first) {
  //   diag(callee->getExprLoc(), "methods with attribute can be called only on members that share the lifetime of this, or have attribute themselves");
  //   return;
  // }

  for (unsigned i = 0; i != decl->param_size(); ++i) {
    auto p = decl->getParamDecl(i);
    if (p->hasAttr<NodeCppMayExtendAttr>()) {

      auto e = call->getArg(i);
      if(isFunctionPtr(e)) {
        continue;
      } else if(tryCheckAsLambda(e, mayExtend)) {
        continue;
      } else {
        if(mayExtend.first) {
          auto ch = NakedPtrScopeChecker::makeThisScopeChecker(this, getContext());
          if(ch.checkExpr(e))
            continue;
        }
        if(auto dr = dyn_cast<DeclRefExpr>(ignoreTemporaries(e))) {
          if(dr->getDecl() == mayExtend.second)
            continue;
        }
      }
	    // e may be null?
      diag(e->getExprLoc(), "is not safe to extend argument scope to 'this'");
    }
  }
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
