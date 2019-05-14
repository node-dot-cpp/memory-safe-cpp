/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
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

#ifndef NODECPP_CHECKER_COROUTINEASTVISITOR_H
#define NODECPP_CHECKER_COROUTINEASTVISITOR_H


#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

namespace nodecpp {
namespace checker {

struct MyStack {
  std::vector<std::vector<const clang::VarDecl*>> Vds;

  void enter() {Vds.push_back({});}
  void exit() {Vds.pop_back();}

  void add(const clang::VarDecl* D) {
    if(!Vds.empty())
      Vds.back().push_back(D);
  }

  class Riia {
    MyStack& St;
    public:
    Riia(MyStack& St) :St(St) {St.enter();}
    ~Riia() {St.exit();}
  }; 

};

class CoroutineASTVisitor
  : public clang::RecursiveASTVisitor<CoroutineASTVisitor> {

  ClangTidyContext &Context;
  MyStack St;

  /// \brief Add a diagnostic with the check's name.
  DiagnosticBuilder diag(SourceLocation Loc, StringRef Message,
                         DiagnosticIDs::Level Level = DiagnosticIDs::Error) {
    return Context.diag(DiagMsgSrc, Loc, Message, Level);
  }

public:

  explicit CoroutineASTVisitor(ClangTidyContext &Context): Context(Context) {}

  bool TraverseDecl(Decl *D) {
    //mb: we don't traverse decls in system-headers
    //TranslationUnitDecl has an invalid location, but needs traversing anyway

    if (!isa<TranslationUnitDecl>(D)) {
      if(isSystemLocation(&Context, D->getLocation()))
        return true;
    }

    return RecursiveASTVisitor<CoroutineASTVisitor>::TraverseDecl(D);
  }

  bool TraverseFunctionDecl(clang::FunctionDecl *D) {

    MyStack::Riia Riia(St);
    return clang::RecursiveASTVisitor<CoroutineASTVisitor>::TraverseFunctionDecl(D);;
  }

  bool TraverseCompoundStmt(clang::CompoundStmt *S) {

    MyStack::Riia Riia(St);
    return clang::RecursiveASTVisitor<CoroutineASTVisitor>::TraverseCompoundStmt(S);;
  }

  bool VisitVarDecl(clang::VarDecl *D) {
    auto Qt = D->getType().getCanonicalType();
    if(isNakedPointerType(Qt, &Context)) {
      St.add(D);
    }
    else if(Qt->isReferenceType()) {
      St.add(D);
    }

    return clang::RecursiveASTVisitor<CoroutineASTVisitor>::VisitVarDecl(D);
  }

  bool VisitCoawaitExpr(clang::CoawaitExpr *E) {
    for(auto &Outer : St.Vds) {
      for(auto &Inner : Outer) {
        diag(Inner->getLocation(), "(S5.8) variable has 'co_await' in its scope");
        diag(E->getExprLoc(), "'co_await' here", DiagnosticIDs::Note);
      }
    }
    return clang::RecursiveASTVisitor<CoroutineASTVisitor>::VisitCoawaitExpr(E);
  }

  bool VisitCoyieldExpr(clang::CoyieldExpr *E) {
    for(auto &Outer : St.Vds) {
      for(auto &Inner : Outer) {
        diag(Inner->getLocation(), "(S5.8) variable has 'co_yield' in its scope");
        diag(E->getExprLoc(), "'co_yield' here", DiagnosticIDs::Note);
      }
    }
    return clang::RecursiveASTVisitor<CoroutineASTVisitor>::VisitCoyieldExpr(E);
  }

};

class CoroutineASTConsumer : public clang::ASTConsumer {

  CoroutineASTVisitor Visitor;

public:
  CoroutineASTConsumer(ClangTidyContext &Context) :Visitor(Context) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

};

} // namespace checker
} // namespace nodecpp

#endif // NODECPP_CHECKER_COROUTINEASTVISITOR_H

