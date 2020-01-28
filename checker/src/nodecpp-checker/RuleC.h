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
//  MyStack St;


  /// \brief Add a diagnostic with the check's name.
  DiagnosticBuilder diag(SourceLocation Loc, StringRef Message,
                         DiagnosticIDs::Level Level = DiagnosticIDs::Error) {
    return Context.diag(DiagMsgSrc, Loc, Message, Level);
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

  bool VisitNamespaceDecl(clang::NamespaceDecl *D) {
    if(!D->isAnonymousNamespace() && !D->isOriginalNamespace()) {

      clang::NamespaceDecl* O = D->getOriginalNamespace();
      if((D->hasAttr<NodeCppMemoryUnsafeAttr>() != O->hasAttr<NodeCppMemoryUnsafeAttr>()) ||
          (D->hasAttr<NodeCppNonDeterministicAttr>() != O->hasAttr<NodeCppNonDeterministicAttr>())) {

        diag(D->getLocation(), "(C3) inconsistent attribute at namespace declaration");
        diag(O->getLocation(), "previous declaration was here", DiagnosticIDs::Note);
      }
    }

    return Super::VisitNamespaceDecl(D);
  }

  bool VisitDecl(clang::Decl *D) {

    if(D->hasAttr<NodeCppMemoryUnsafeAttr>() && !isa<NamespaceDecl>(D)) {
      diag(D->getLocation(), "(C2) attribute [[nodecpp::memory_unsafe]] allowed at namespace declaration only");
    }

    if(D->hasAttr<NodeCppNonDeterministicAttr>() && !isa<NamespaceDecl>(D)) {
      diag(D->getLocation(), "(C2) attribute [[nodecpp::non_determinstic]] allowed at namespace declaration only");
    }

    if(D->hasAttr<NodeCppNakedStructAttr>()) {
      auto R = dyn_cast<CXXRecordDecl>(D);
      if(!R || (!R->isStruct() && !R->isClass())) {
        diag(D->getLocation(), "(C2) attribute [[nodecpp::naked_struct]] allowed at struct or class declaration only");
      }
    }

    if(D->hasAttr<NodeCppMayExtendAttr>()) {
      auto P = dyn_cast<ParmVarDecl>(D);
      if(!P) {
        diag(D->getLocation(), "(C2) attribute [[nodecpp::may_extend_to_this]] allowed at non-static, non-lambda, method argument declaration only");
      }
      else {
        auto M = dyn_cast<CXXMethodDecl>(D->getDeclContext());
        if(!M || M->isStatic() || isLambdaCallOperator(M)) {
          diag(D->getLocation(), "(C2) attribute [[nodecpp::may_extend_to_this]] allowed at non-static, non-lambda, method argument declaration only");
        }
      }
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

