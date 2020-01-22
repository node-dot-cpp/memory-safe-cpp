/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- VarDeclCheck.cpp - clang-tidy-------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "VarDeclCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void VarDeclCheck::registerMatchers(MatchFinder *Finder) {

  // Finder->addMatcher(
  //     varDecl(unless(anyOf(
  //       hasParent(cxxConstructorDecl()),
  //                          hasParent(cxxMethodDecl(isCopyAssignmentOperator())),

  //                          hasParent(cxxMethodDecl(isMoveAssignmentOperator()))
  //                          )))
  //         .bind("var"),
  //     this);

  Finder->addMatcher(varDecl().bind("var"), this);
}

const CXXMethodDecl *VarDeclCheck::getParentMethod(ASTContext *Context,
                                                   const VarDecl *Var) {

  auto SList = Context->getParents(*Var);

  for (auto SIt = SList.begin(); SIt != SList.end(); ++SIt) {
    if (auto T = SIt->get<TypeLoc>()) {
      auto SList2 = Context->getParents(*T);
      for (auto SIt2 = SList2.begin(); SIt2 != SList2.end(); ++SIt2) {
        if (auto D = SIt2->get<CXXMethodDecl>())
          return D;
      }
    } else if (auto D = SIt->get<CXXMethodDecl>())
      return D;
  }

  return nullptr;
}

void VarDeclCheck::check(const MatchFinder::MatchResult &Result) {

  auto Var = Result.Nodes.getNodeAs<VarDecl>("var");
  assert(Var);

  // any type will also pop-up at constructor and operator assignment
  // declaration, we let both of them go
  auto P = getParentMethod(Result.Context, Var);
  if (P) {
    if (isa<CXXConstructorDecl>(P))
      return;

    if (auto M = dyn_cast<CXXMethodDecl>(P)) {
      if (M->isCopyAssignmentOperator() || M->isMoveAssignmentOperator())
        return;
    }
  }

  bool IsParam = isa<ParmVarDecl>(Var);
  auto Qt = Var->getType().getCanonicalType();

  //first check references initializers
  if (Qt->isReferenceType() && !IsParam) {
    auto E = Var->getInit();
    if (!E) {
      // this is actually a build error
      diag(Var->getLocation(),
           "(S5.3) reference without initializer is prohibited");
      return;
    }

    if (isa<ExprWithCleanups>(E)) {
      diag(E->getExprLoc(),
           "(S5.3) reference with temporary initializer is prohibited");
      return;
    }
  }

  //unwrap const ref
  if (Qt->isReferenceType()) {
    if (Qt->getPointeeType().isConstQualified()) {
      Qt = Qt->getPointeeType().getCanonicalType();
      Qt.removeLocalConst();
    }
  }

  if (auto U = isUnionType(Qt)) {
    if (!checkUnion(U)) {
      auto Dh = DiagHelper(this);
      Dh.diag(Var->getLocation(), "unsafe union at variable declaration");
      checkUnion(U, Dh);
    }
    return;
  }

  if (isSafeType(Qt, getContext())) {
    return;
  }

  if (Qt->isReferenceType()) {
    //non-const reference only from safe types
    QualType Inner = Qt->getPointeeType().getCanonicalType();
    if (!isSafeType(Inner, getContext())) {
      diag(Var->getLocation(),
           "(S5.3) non-const reference of unsafe type is prohibited");
    }

    //this is all for non-const references
    return;
  }

  if (isAnyFunctorType(Var->getType())) // don't use canonical type here
    return;

  if (isAwaitableType(Qt)) {
    if(IsParam)
      diag(Var->getLocation(), "(S9.1) awaitable parameter not allowed");
    else
      diag(Var->getLocation(), "(S9.1) awaitable variable not allowed (yet)");

    return;
  }

  if (isRawPointerType(Qt)) {
    //getContext()->getGlobalOptions().SafeFunctions;
    bool Allow = false;
    if (!Allow) {
      diag(Var->getLocation(), "(S1.3) raw pointer declaration is prohibited");
      return;
    }

    QualType Inner = Qt->getPointeeType().getCanonicalType();
    if (!isSafeType(Inner, getContext())) {
      diag(Var->getLocation(),
           "(S5.3) raw pointer of unsafe type is prohibited");
      return;
    }

    // //params don't need initializer
    if (!IsParam) {
      auto E = Var->getInit();
      if (!E) {
        diag(Var->getLocation(),
             "(S5.3) raw pointer variable type must have initializer");
        return;
      }
    }

    //this is all for raw pointer
    return;
  }

  if (auto Np = isNakedPointerType(Qt, getContext())) {
    if (Np.isOk()) {
      //this is all for naked_ptr
      return;
    }

    auto Dh = DiagHelper(this);
    Dh.diag(Var->getLocation(), "unsafe naked_ptr at variable declaration");
    isNakedPointerType(Qt, getContext(), Dh); // for report
    return;
  }

  if (auto Ns = isNakedStructType(Qt, getContext())) {
    if (Ns.isOk()) {
      if (Var->hasAttr<NodeCppMayExtendAttr>()) {
        diag(Var->getLocation(),
             "may_extend not implemented for naked struct variables yet");
      }
      return;
    }

    auto Dh = DiagHelper(this);
    Dh.diag(Var->getLocation(), "unsafe naked_struct at variable declaration");
    isNakedStructType(Qt, getContext(), Dh); // for report
    return;
  }

  if (isLambdaType(Qt)) {

    //naked struct internal is checked at other place
    if (Var->hasAttr<NodeCppMayExtendAttr>()) {
      diag(Var->getLocation(), "may_extend not implemented for lambda");
    }

    // this is all for iplicit naked struct
    return;
  }

  auto Dh = DiagHelper(this);
  Dh.diag(Var->getLocation(), "unsafe type at variable declaration");
  isSafeType(Qt, getContext(), Dh);

  return;
}

} // namespace checker
} // namespace nodecpp
