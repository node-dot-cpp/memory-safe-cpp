/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#include "ConsistencyRule.h"
#include "CheckerASTVisitor.h"
#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTLambda.h"

namespace nodecpp {
namespace checker {




class RuleCASTVisitor
  : public CheckerASTVisitor<RuleCASTVisitor> {

  bool IsMemoryUnsafe = false;
//  MyStack St;

  /// \brief Add a diagnostic with the check's name.
  DiagnosticBuilder diag(SourceLocation Loc, StringRef Message,
                         DiagnosticIDs::Level Level = DiagnosticIDs::Error) {
    return Context->diag(DiagMsgSrc, Loc, Message, Level);
  }

  CheckHelper* getCheckHelper() const { return Context->getCheckHelper(); }

  template<typename ATTR>
  void checkConsistency(clang::Decl *Current, clang::Decl *Reference, StringRef AttrName) {
    if(Current->hasAttr<ATTR>() != Reference->hasAttr<ATTR>()) {
      diag(Current->getLocation(), "(C3) inconsistent attribute %0 at declaration") << AttrName;
      diag(Reference->getLocation(), "previous declaration was here", DiagnosticIDs::Note);
    }
  } 

  void diagC2(SourceLocation Loc, StringRef AttrName, StringRef Where) {
      diag(Loc, "(C2) attribute %0 allowed at %1 only") << AttrName << Where;
  }
public:

  explicit RuleCASTVisitor(ClangTidyContext *Context):
    CheckerASTVisitor<RuleCASTVisitor>(Context) {}

  bool TraverseNamespaceDecl(clang::NamespaceDecl *D) {

    if(D->hasAttr<SafeMemoryMemoryUnsafeAttr>()) {
              
      FlagRiia R(IsMemoryUnsafe);
      return Super::TraverseNamespaceDecl(D);
    }
    else
      return Super::TraverseNamespaceDecl(D);
  }


  bool VisitNamespaceDecl(clang::NamespaceDecl *D) {

    if(D->isAnonymousNamespace()) {

      //nothing here
    }
    else if(D->isOriginalNamespace()) {
      if(D->hasAttr<SafeMemoryMemoryUnsafeAttr>()) {

        std::string Name = getQnameForSystemSafeDb(D);
        getCheckHelper()->addUnsafeNamespace(Name);
      }
    }
    else {
      clang::NamespaceDecl* O = D->getOriginalNamespace();
      checkConsistency<SafeMemoryMemoryUnsafeAttr>(D, O, "[[safememory::memory_unsafe]]");
      checkConsistency<SafeMemoryNonDeterministicAttr>(D, O, "[[safememory::non_deterministic]]");
    }

    return Super::VisitNamespaceDecl(D);
  }

  bool VisitFunctionDecl(clang::FunctionDecl *D) {

    if(!D->isCanonicalDecl()) {
      clang::FunctionDecl* C = D->getCanonicalDecl();
      checkConsistency<SafeMemoryNoSideEffectAttr>(D, C, "[[safememory::no_side_effect]]");
    }

    return Super::VisitFunctionDecl(D);
  }

  bool VisitCXXRecordDecl(clang::CXXRecordDecl *D) {

    if(!D->isCanonicalDecl()) {
      clang::CXXRecordDecl* C = D->getCanonicalDecl();
      checkConsistency<SafeMemoryNakedStructAttr>(D, C, "[[safememory::naked_struct]]");
      checkConsistency<SafeMemoryDeepConstAttr>(D, C, "[[safememory::deep_const]]");
    }

    return Super::VisitCXXRecordDecl(D);
  }

  bool VisitDecl(clang::Decl *D) {

    if(D->hasAttr<SafeMemoryMemoryUnsafeAttr>() && !isa<NamespaceDecl>(D)) {
      diagC2(D->getLocation(), "[[safememory::memory_unsafe]]", "namespace");
    }

    if(D->hasAttr<SafeMemoryNonDeterministicAttr>() && !isa<NamespaceDecl>(D)) {
      diagC2(D->getLocation(), "[[safememory::non_determinstic]]", "namespace");
    }

    if(D->hasAttr<SafeMemoryNakedStructAttr>()) {
      auto R = dyn_cast<CXXRecordDecl>(D);
      if(R && (R->isStruct() || R->isClass())) {
        ; //ok
      }
      else {
        diagC2(D->getLocation(), "[[safememory::naked_struct]]", "struct or class");
      }
    }

    if(D->hasAttr<SafeMemoryDeepConstAttr>()) {
      auto R = dyn_cast<CXXRecordDecl>(D);
      if(R && (R->isStruct() || R->isClass())) {
        ;//ok
      }
      else {
        diagC2(D->getLocation(), "[[safememory::deep_const]]", "struct or class");
      }
    }

    if(D->hasAttr<SafeMemoryNoSideEffectAttr>()) {
      if(isa<CXXConstructorDecl>(D))
        diagC2(D->getLocation(), "[[safememory::no_side_effect]]", "non-virtual function");
      if(!isa<FunctionDecl>(D))
        diagC2(D->getLocation(), "[[safememory::no_side_effect]]", "non-virtual function");
      else if(auto M = dyn_cast<CXXMethodDecl>(D)) {
        if(M->isVirtual())
          diagC2(D->getLocation(), "[[safememory::no_side_effect]]", "non-virtual function");
      }
    }


    if(D->hasAttr<SafeMemoryMayExtendAttr>() && !IsMemoryUnsafe) {
      diagC2(D->getLocation(), "[[safememory::may_extend_to_this]]", "system libraries");
    }

    if(D->hasAttr<SafeMemoryNoAwaitAttr>() && !IsMemoryUnsafe) {
      diagC2(D->getLocation(), "[[safememory::no_await]]", "system libraries");
    }


    if(D->hasAttr<SafeMemoryNoSideEffectWhenConstAttr>() && !IsMemoryUnsafe) {
      diagC2(D->getLocation(), "[[safememory::no_side_effect_when_const]]", "system libraries");
    }

    if(D->hasAttr<SafeMemoryDeepConstWhenParamsAttr>() && !IsMemoryUnsafe) {
      diagC2(D->getLocation(), "[[safememory::deep_const_when_params]]", "system libraries");
    }

    if(D->hasAttr<SafeMemoryAwaitableAttr>() && !IsMemoryUnsafe) {
      diagC2(D->getLocation(), "[[safememory::awaitable]]", "system libraries");
    }

    return Super::VisitDecl(D);
  }

};

class RuleCASTConsumer : public clang::ASTConsumer {

  RuleCASTVisitor Visitor;

public:
  RuleCASTConsumer(ClangTidyContext *Context) :Visitor(Context) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

};

std::unique_ptr<clang::ASTConsumer> makeConsistencyRule(ClangTidyContext *Context) {
  return llvm::make_unique<RuleCASTConsumer>(Context);
}


} // namespace checker
} // namespace nodecpp


