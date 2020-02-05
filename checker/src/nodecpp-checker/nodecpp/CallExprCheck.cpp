/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- CallExprCheck.cpp - clang-tidy------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CallExprCheck.h"
#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

void CallExprCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(callExpr().bind("call"), this);
}

std::string CallExprCheck::getQnameForSafeLibraryDb(const NamedDecl *Decl) {

  std::string QualName;
  llvm::raw_string_ostream OS(QualName);

  const DeclContext *Ctx = Decl->getDeclContext();

  if (Ctx->isFunctionOrMethod()) {
    // printName(OS);
    return "";
  }

  using ContextsTy = SmallVector<const DeclContext *, 8>;
  ContextsTy Contexts;

  // Collect named contexts.
  while (Ctx) {
    if (isa<NamedDecl>(Ctx))
      Contexts.push_back(Ctx);
    Ctx = Ctx->getParent();
  }

  for (const DeclContext *DC : llvm::reverse(Contexts)) {
    if (const auto *Spec = dyn_cast<ClassTemplateSpecializationDecl>(DC)) {
      OS << Spec->getName();
      // const TemplateArgumentList &TemplateArgs = Spec->getTemplateArgs();
      // printTemplateArgumentList(OS, TemplateArgs.asArray(), P);
    } else if (const auto *ND = dyn_cast<NamespaceDecl>(DC)) {
      if (/* P.SuppressUnwrittenScope && */
          (ND->isAnonymousNamespace() || ND->isInline()))
        return "";
      // if (ND->isAnonymousNamespace()) {
      //   OS << (P.MSVCFormatting ? "`anonymous namespace\'"
      //                           : "(anonymous namespace)");
      // }
      else
        OS << *ND;
    } else if (const auto *RD = dyn_cast<RecordDecl>(DC)) {
      if (!RD->getIdentifier())
        // OS << "(anonymous " << RD->getKindName() << ')';
        return "";
      else
        OS << *RD;
    } else if (const auto *FD = dyn_cast<FunctionDecl>(DC)) {
      // const FunctionProtoType *FT = nullptr;
      // if (FD->hasWrittenPrototype())
      //   FT = dyn_cast<FunctionProtoType>(FD->getType()->castAs<FunctionType>());

      // OS << *FD << '(';
      // if (FT) {
      //   unsigned NumParams = FD->getNumParams();
      //   for (unsigned i = 0; i < NumParams; ++i) {
      //     if (i)
      //       OS << ", ";
      //     OS << FD->getParamDecl(i)->getType().stream(P);
      //   }

      //   if (FT->isVariadic()) {
      //     if (NumParams > 0)
      //       OS << ", ";
      //     OS << "...";
      //   }
      // }
      // OS << ')';
      return "";
    } else if (const auto *ED = dyn_cast<EnumDecl>(DC)) {
      // C++ [dcl.enum]p10: Each enum-name and each unscoped
      // enumerator is declared in the scope that immediately contains
      // the enum-specifier. Each scoped enumerator is declared in the
      // scope of the enumeration.
      // For the case of unscoped enumerator, do not include in the qualified
      // name any information about its enum enclosing scope, as its visibility
      // is global.
      if (ED->isScoped())
        OS << *ED;
      else
        return "";
    } else {
      OS << *cast<NamedDecl>(DC);
    }
    OS << "::";
  }

  if (Decl->getDeclName() || isa<DecompositionDecl>(Decl))
    OS << *Decl;
  else
    return "";
//    OS << "(anonymous)";

  return OS.str();

}


void CallExprCheck::check(const MatchFinder::MatchResult &Result) {

  auto Ex = Result.Nodes.getNodeAs<CallExpr>("call");

  auto Decl = Ex->getDirectCallee();
  if (!Decl)
    return;

  SourceManager *Manager = Result.SourceManager;
  auto ELoc = Manager->getExpansionLoc(Decl->getLocation());

  if(!isSystemLocation(getContext(), ELoc))
    return; // this is in user code, then is ok

  std::string Name = getQnameForSafeLibraryDb(Decl);

  if(isSoftPtrCastName(Name)) {
    diag(Ex->getExprLoc(), "(S1.1.1) soft_ptr cast is prohibited in safe code");
    return;
  }

  if(isSystemSafeFunctionName(getContext(), Name))
    return;

  diag(Ex->getExprLoc(),
       "(S8) function call '" + Name + "' is not listed as safe, therefore is prohibited");
}

} // namespace checker
} // namespace nodecpp
