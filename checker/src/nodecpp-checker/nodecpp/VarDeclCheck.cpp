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

  bool IsParam = isParmVarOrCatchVar(Result.Context, Var);
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
      auto Dh = DiagHelper(getContext());
      getContext()->diagError2(Var->getLocation(), "safe-union", "unsafe union at variable declaration");
      checkUnion(U, Dh);
    }
    return;
  }

  //here we verify that deep const template instantiations are indeed deep-const
  if(auto Dc = getCheckHelper()->checkDeepConst(Qt)) {
    if(!Dc.isOk()) {
      getContext()->diagError2(Var->getLocation(), "deep-const", "unsafe deep_const attribute at variable declaration");
      getCheckHelper()->reportDeepConstDetail(Qt);
      return;
    }
  }



  if (getCheckHelper()->isHeapSafe(Qt)) {
    return;
  }

  if (Qt->isReferenceType()) {
    //non-const reference only from safe types
    QualType Inner = Qt->getPointeeType();
    if (!getCheckHelper()->isHeapSafe(Inner)) {
      diag(Var->getLocation(),
           "(S5.3) non-const reference of unsafe type is prohibited");
    }

    //this is all for non-const references
    return;
  }

  if (isStdFunctionType(Qt))
    return;

  if (isAwaitableType(Qt)) {

    //don't diagnose here
    // if(IsParam)
    //   diag(Var->getLocation(), "(S9) awaitable parameter not allowed");
    // else
    //   diag(Var->getLocation(), "(S9) awaitable variable not allowed (yet)");

    return;
  }

  if (isRawPointerType(Qt)) {

    bool NoRawPtr = getContext()->getGlobalOptions().DisableRawPointers;
    if (NoRawPtr) {
      diag(Var->getLocation(), "(S1.3) raw pointer declaration is prohibited");
      return;
    }

    QualType Inner = Qt->getPointeeType();
    if (!getCheckHelper()->isHeapSafe(Inner)) {
      diag(Var->getLocation(),
           "(S5.3) raw pointer of unsafe type is prohibited");
      return;
    }

    if (!IsParam) {
      auto Ex = Var->getInit();
      if (!Ex) {
        diag(Var->getLocation(),
             "(S1.3) raw pointer variable type must have initializer");
        return;
      }
    }

    // if (auto P = dyn_cast<ParmVarDecl>(Var)) {
    //   // params check they don't have a null default initializer
    //   auto Ex = P->getDefaultArg();
    //   if(Ex && isNullPtrValue(getASTContext(), Ex)) {
    //     diag(Var->getLocation(),
    //          "(S1.3) raw pointer parameter can't be initialized to null");
    //     return;
    //   }
    // }
    // else {
    //   auto Ex = Var->getInit();
    //   if (!Ex) {
    //     diag(Var->getLocation(),
    //          "(S1.3) raw pointer variable type must have initializer");
    //     return;
    //   }
    //   else if(isNullPtrValue(getASTContext(), Ex)) {
    //     diag(Var->getLocation(),
    //          "(S1.3) raw pointer variable can't be initialized to null");
    //     return;
    //   }
    // }

    //this is all for raw pointer
    return;
  }

  if (auto Np = getCheckHelper()->checkNullablePtr(Qt)) {
    if (Np.isOk()) {
      //this is all for naked_ptr
      return;
    }

    getContext()->diagError2(Var->getLocation(), "nullable-ptr", "unsafe nullable_ptr at variable declaration");
    getCheckHelper()->reportNullablePtrDetail(Qt);
    return;
  }

  if (auto Ns = getCheckHelper()->checkNakedStruct(Qt)) {
    if (Ns.isOk()) {
      if (Var->hasAttr<NodeCppMayExtendAttr>() || Var->hasAttr<SafeMemoryMayExtendAttr>()) {
        diag(Var->getLocation(),
             "may_extend not implemented for naked struct variables yet");
      }
      return;
    }

    // auto Dh = DiagHelper(getContext());
    getContext()->diagError2(Var->getLocation(), "naked-struct", "unsafe naked_struct at variable declaration");
    getCheckHelper()->reportNakedStructDetail(Qt); // for report
    return;
  }
  
  if (isLambdaType(Qt)) {

    //naked struct internal is checked at other place
    if (Var->hasAttr<NodeCppMayExtendAttr>() || Var->hasAttr<SafeMemoryMayExtendAttr>()) {
      diag(Var->getLocation(), "may_extend not implemented for lambda");
    }

    // this is all for iplicit naked struct
    return;
  }

  getContext()->diagError2(Var->getLocation(), "heap-safe", "unsafe type at variable declaration");
  getCheckHelper()->reportNonSafeDetail(Qt);

  return;
}

} // namespace checker
} // namespace nodecpp
