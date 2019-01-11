/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- NewExprCheck.cpp - clang-tidy-------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NakedPtrHelper.h"
#include "NewExprCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace nodecpp {

void NewExprCheck::registerMatchers(MatchFinder *Finder) {

  if (!getLangOpts().CPlusPlus)
    return;

  Finder->addMatcher(
      cxxNewExpr().bind("new"), this);
  Finder->addMatcher(
      cxxDeleteExpr().bind("delete"), this);

  Finder->addMatcher(
      callExpr(callee(functionDecl(hasName("nodecpp::make_owning"))))
          .bind("make"),
      this);
}

void NewExprCheck::check(const MatchFinder::MatchResult &Result) {

  if(auto expr = Result.Nodes.getNodeAs<CXXNewExpr>("new")) {

    diag(expr->getExprLoc(), "(S4) operator new is prohibited");
  }
  else if(auto expr = Result.Nodes.getNodeAs<CXXDeleteExpr>("delete")) {

    diag(expr->getExprLoc(), "(S4) operator delete is prohibited");
  }
  else if(auto expr = Result.Nodes.getNodeAs<CallExpr>("make")) {

    auto parent = getParentExpr(Result.Context, expr);
    if (parent) {
//      parent->dumpColor();
      if (auto constructor = dyn_cast<CXXConstructExpr>(parent)) {
        auto decl = constructor->getConstructor()->getParent();

        if (isOwningPtrRecord(decl)) {
          // this is ok!
          return;
        }
      } else if (auto member = dyn_cast<CXXMemberCallExpr>(parent)) {

        auto decl = member->getMethodDecl()->getParent();
        if(isOwningPtrRecord(decl)) {
          // this is ok!
          return;
        }
      }
      else if(auto op = dyn_cast<CXXOperatorCallExpr>(parent)) {
        auto opDecl = dyn_cast<CXXMethodDecl>(op->getDirectCallee());
        if (opDecl && isOwningPtrRecord(opDecl->getParent())) {
          if(opDecl->isMoveAssignmentOperator() 
            || opDecl->isCopyAssignmentOperator()) {
              // this is ok!
              return;
          }
        }
      }
    }

    diag(expr->getExprLoc(), "(S4.1) result of make_owning must be assigned to owning_ptr");
  }

  // if (isNoInstanceType(m->getAllocatedType())) {
  //   diag(m->getLocStart(),
  //        "type with attribute can be instantiated from safe code");
  //   return;
  // }

  // type is ok, verify is owned by unique_ptr
/*
  auto parent = getParentExpr(Result.Context, m);
  if (parent) {
//    parent->dumpColor();
    if (auto constructor = dyn_cast<CXXConstructExpr>(parent)) {
      auto decl = constructor->getConstructor()->getParent();
      auto name = decl->getQualifiedNameAsString();
      if (isOwnerPtrName(name)) {
        // this is ok!
        return;
      }
    } else if (auto member = dyn_cast<CXXMemberCallExpr>(parent)) {

      auto method = member->getMethodDecl();
      auto decl = member->getMethodDecl()->getParent();
      auto methodName = method->getNameAsString();
      auto className = decl->getQualifiedNameAsString();
      if (methodName == "reset" && isOwnerPtrName(className)) {
        // this is ok!
        return;
      }
    }
  }
*/
  //diag(m->getLocStart(), "new expresion must be owned by a unique_ptr");
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
