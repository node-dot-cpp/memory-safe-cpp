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

namespace clang {
namespace tidy {
namespace nodecpp {

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

const CXXMethodDecl *VarDeclCheck::getParentMethod(ASTContext *context, const VarDecl *var) {

  auto sList = context->getParents(*var);

  for(auto sIt = sList.begin(); sIt != sList.end(); ++sIt) {
    if(auto t = sIt->get<TypeLoc>()) {
      auto sList2 = context->getParents(*t);
      for(auto sIt2 = sList2.begin(); sIt2 != sList2.end(); ++sIt2) {
        if (auto d = sIt2->get<CXXMethodDecl>())
          return d;
      }
    }
    else if (auto d = sIt->get<CXXMethodDecl>())
      return d;
  }

  return nullptr;
}


void VarDeclCheck::check(const MatchFinder::MatchResult &Result) {

  auto var = Result.Nodes.getNodeAs<VarDecl>("var");
  assert(var);

  // any type will also pop-up at constructor and operator assignment
  // declaration, we let both of them go
  auto p = getParentMethod(Result.Context, var);
  if(p) {
    if(isa<CXXConstructorDecl>(p))
      return;

    if(auto m = dyn_cast<CXXMethodDecl>(p)) {
      if(m->isCopyAssignmentOperator() || m->isMoveAssignmentOperator())
        return;
    }
  }

//  bool isParam = isa<ParmVarDecl>(var);
  auto qt = var->getType().getCanonicalType();

  //unwrap const ref
  if(qt->isReferenceType() && qt.isConstQualified())
    qt = qt->getPointeeType().getCanonicalType();
  
  if(auto u = isUnionType(qt)) {
    if(!checkUnion(u)) {
      auto dh = DiagHelper(this);
      dh.diag(var->getLocation(), "unsafe union at variable declaration");
      checkUnion(u, dh);
    }
    return;
  }

  if (isSafeType(qt, getContext())) {
    return;
  }

  if(qt->isReferenceType()) {
    //non-const reference only from safe types
    QualType inner = qt->getPointeeType().getCanonicalType();
    if(!isSafeType(inner, getContext())) {
      diag(var->getLocation(), "(S5.3) non-const reference of unsafe type is prohibited");
    }

    //this is all for non-const references
    return;
  }

  if(isAnyFunctionType(qt))
    return;

  if(isRawPointerType(qt)) {
    // if(!checkRawPointerType(qt, this)) {
    //   diag(var->getLocation(), "Unsafe raw pointer declaration");
    //   return;
    // }

    
    // //params don't need initializer
    // if(isParam) {
    //   diag(var->getLocation(), "(S1.3) raw pointer declaration is prohibited");
    //   return;
    // }
    
    // auto e = var->getInit();
    // if(!e) {
    //   diag(var->getLocation(), "raw pointer type must have initializer");
    //   return;
    // }

    //this is all for raw pointer
    diag(var->getLocation(), "(S1.3) raw pointer declaration is prohibited");
    return;
  }


  if(auto np = isNakedPointerType(qt, getContext())) {
    if(np.isOk()) {
      //this is all for naked_ptr
      return;

    }

    auto dh = DiagHelper(this);
    dh.diag(var->getLocation(), "unsafe naked_ptr at variable declaration");
    isNakedPointerType(qt, getContext(), dh); //for report
    return;
  }

  if(auto ns = isNakedStructType(qt, getContext())) {
    if(ns.isOk()) {
      if(var->hasAttr<NodeCppMayExtendAttr>()) {
        diag(var->getLocation(), "may_extend not implemented for naked struct variables yet");
      }
      return;
    }

    auto dh = DiagHelper(this);
    dh.diag(var->getLocation(), "unsafe naked_struct at variable declaration");
    isNakedStructType(qt, getContext(), dh); //for report
    return;
  }

  if(isLambdaType(qt)) {

    //naked struct internal is checked at other place
    if(var->hasAttr<NodeCppMayExtendAttr>()) {
      diag(var->getLocation(), "may_extend not implemented for lambda");
    }

    // this is all for iplicit naked struct
    return;
  }

  auto dh = DiagHelper(this);
  dh.diag(var->getLocation(), "unsafe type at variable declaration");
  isSafeType(qt, getContext(), dh);

  return;
}

} // namespace nodecpp
} // namespace tidy
} // namespace clang
