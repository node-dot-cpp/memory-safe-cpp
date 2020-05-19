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

#ifndef NODECPP_CHECKER_RULECASTVISITOR_H
#define NODECPP_CHECKER_RULECASTVISITOR_H


#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTLambda.h"

namespace nodecpp {
namespace checker {




class RuleCASTVisitor
  : public clang::RecursiveASTVisitor<RuleCASTVisitor> {

  typedef clang::RecursiveASTVisitor<RuleCASTVisitor> Super;

  ClangTidyContext &Context;
  bool IsMemoryUnsafe = false;
//  MyStack St;

  struct FlagRiia {
    bool &V;
    bool OldValue;
    FlagRiia(bool &V) :V(V) {
      OldValue = V;
      V = true;
    }
    ~FlagRiia() {
      V = OldValue;
    }

  };

  /// \brief Add a diagnostic with the check's name.
  DiagnosticBuilder diag(SourceLocation Loc, StringRef Message,
                         DiagnosticIDs::Level Level = DiagnosticIDs::Error) {
    return Context.diag(DiagMsgSrc, Loc, Message, Level);
  }

  CheckHelper* getCheckHelper() const { return Context.getCheckHelper(); }

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

  explicit RuleCASTVisitor(ClangTidyContext &Context): Context(Context) {}

  bool TraverseDecl(Decl *D) {
    //mb: we don't traverse decls in system-headers
    //TranslationUnitDecl has an invalid location, but needs traversing anyway

    if(!D)
      return true;

    else if (isa<TranslationUnitDecl>(D))
      return Super::TraverseDecl(D);

    else if(isSystemLocation(&Context, D->getLocation()))
        return true;

    else
      return Super::TraverseDecl(D);
  }

  bool TraverseNamespaceDecl(clang::NamespaceDecl *D) {

    if(D->hasAttr<NodeCppMemoryUnsafeAttr>()) {
              
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
      if(D->hasAttr<NodeCppMemoryUnsafeAttr>()) {

        std::string Name = getQnameForSystemSafeDb(D);
        getCheckHelper()->addUnsafeNamespace(Name);
      }
    }
    else {
      clang::NamespaceDecl* O = D->getOriginalNamespace();
      checkConsistency<NodeCppMemoryUnsafeAttr>(D, O, "[[memory_unsafe]]");
      checkConsistency<NodeCppNonDeterministicAttr>(D, O, "[[non_deterministic]]");
    }

    return Super::VisitNamespaceDecl(D);
  }

  bool VisitFunctionDecl(clang::FunctionDecl *D) {

    if(!D->isCanonicalDecl()) {
      clang::FunctionDecl* C = D->getCanonicalDecl();
      checkConsistency<NodeCppNoSideEffectAttr>(D, C, "[[no_side_effect]]");
    }

    return Super::VisitFunctionDecl(D);
  }

  bool VisitCXXRecordDecl(clang::CXXRecordDecl *D) {

    if(!D->isCanonicalDecl()) {
      clang::CXXRecordDecl* C = D->getCanonicalDecl();
      checkConsistency<NodeCppNakedStructAttr>(D, C, "[[naked_struct]]");
      checkConsistency<NodeCppDeepConstAttr>(D, C, "[[deep_const]]");
    }

    return Super::VisitCXXRecordDecl(D);
  }

  bool VisitDecl(clang::Decl *D) {

    if(D->hasAttr<NodeCppMemoryUnsafeAttr>() && !isa<NamespaceDecl>(D)) {
      diagC2(D->getLocation(), "[[nodecpp::memory_unsafe]]", "namespace");
    }

    if(D->hasAttr<NodeCppNonDeterministicAttr>() && !isa<NamespaceDecl>(D)) {
      diagC2(D->getLocation(), "[[nodecpp::non_determinstic]]", "namespace");
    }

    if(D->hasAttr<NodeCppNakedStructAttr>()) {
      auto R = dyn_cast<CXXRecordDecl>(D);
      if(R && (R->isStruct() || R->isClass())) {
        ; //ok
      }
      else {
        diagC2(D->getLocation(), "[[nodecpp::naked_struct]]", "struct or class");
      }
    }

    if(D->hasAttr<NodeCppDeepConstAttr>()) {
      auto R = dyn_cast<CXXRecordDecl>(D);
      if(R && (R->isStruct() || R->isClass())) {
        ;//ok
      }
      else {
        diagC2(D->getLocation(), "[[nodecpp::deep_const]]", "struct or class");
      }
    }

    if(D->hasAttr<NodeCppNoSideEffectAttr>()) {
      if(isa<CXXConstructorDecl>(D))
        diagC2(D->getLocation(), "[[nodecpp::no_side_effect]]", "non-virtual function");
      if(!isa<FunctionDecl>(D))
        diagC2(D->getLocation(), "[[nodecpp::no_side_effect]]", "non-virtual function");
      else if(auto M = dyn_cast<CXXMethodDecl>(D)) {
        if(M->isVirtual())
          diagC2(D->getLocation(), "[[nodecpp::no_side_effect]]", "non-virtual function");
      }
    }


    if(D->hasAttr<NodeCppMayExtendAttr>() && !IsMemoryUnsafe) {
      diagC2(D->getLocation(), "[[nodecpp::may_extend_to_this]]", "system libraries");
    }

    if(D->hasAttr<NodeCppNoAwaitAttr>() && !IsMemoryUnsafe) {
      diagC2(D->getLocation(), "[[nodecpp::no_await]]", "system libraries");
    }


    if(D->hasAttr<NodeCppNoSideEffectWhenConstAttr>() && !IsMemoryUnsafe) {
      diagC2(D->getLocation(), "[[nodecpp::no_side_effect_when_const]]", "system libraries");
    }

    return Super::VisitDecl(D);
  }

};

class RuleCASTConsumer : public clang::ASTConsumer {

  RuleCASTVisitor Visitor;

public:
  RuleCASTConsumer(ClangTidyContext &Context) :Visitor(Context) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

};

} // namespace checker
} // namespace nodecpp

#endif // NODECPP_CHECKER_RULECASTVISITOR_H

