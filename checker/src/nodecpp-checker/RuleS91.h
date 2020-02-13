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

#ifndef NODECPP_CHECKER_RULES91_H
#define NODECPP_CHECKER_RULES91_H


#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTLambda.h"

namespace nodecpp {
namespace checker {




class RuleS91ASTVisitor
  : public clang::RecursiveASTVisitor<RuleS91ASTVisitor> {

  typedef clang::RecursiveASTVisitor<RuleS91ASTVisitor> Super;

  ClangTidyContext &Context;
  llvm::SmallSet<CallExpr*, 4> CallWhiteList;
  llvm::SmallSet<CoawaitExpr*, 4> CoawaitWhiteList;
  llvm::SmallSet<ValueDecl*, 4> DeclWhiteList;
  llvm::SmallSet<Stmt*, 4> DontTraverseCoawaitedExpr;
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

public:

  explicit RuleS91ASTVisitor(ClangTidyContext &Context): Context(Context) {}

  void ReportAndClear() {
    for(auto each : DeclWhiteList) {
      diag(each->getLocation(), "(S9.1) awaitable variable must be awaited");
    }
    DeclWhiteList.clear();
    
    for(auto each : CallWhiteList) {
      each->dump();
      diag(each->getExprLoc(), "(S9.1) internal check failed");
    }
    CallWhiteList.clear();
    
    for(auto each : CoawaitWhiteList) {
      each->dump();
      diag(each->getExprLoc(), "(S9.1) internal check failed");
    }
    CoawaitWhiteList.clear();
    
    for(auto each : DontTraverseCoawaitedExpr) {
      each->dump();
      diag(each->getBeginLoc(), "(S9.1) internal check failed");
    }
    DontTraverseCoawaitedExpr.clear();
  }

  bool TraverseDecl(Decl *D) {
    //mb: we don't traverse decls in system-headers
    //TranslationUnitDecl has an invalid location, but needs traversing anyway

    if(!D)
      return true;

    else if (isa<TranslationUnitDecl>(D))
      return Super::TraverseDecl(D);

    else if(isSystemLocation(&Context, D->getLocation()))
        return true;

    else if(D->hasAttr<NodeCppMemoryUnsafeAttr>())
      return true;

    else
      return Super::TraverseDecl(D);
  }

  bool TraverseStmt(clang::Stmt *St) {

    if(DontTraverseCoawaitedExpr.erase(St))
      return true;
    else
      return Super::TraverseStmt(St);
  }

  void checkWaitForAllCall(clang::Expr *E, bool AddCallsToWhiteList) {

    clang::CallExpr *Ce0 = dyn_cast<clang::CallExpr>(E->IgnoreImplicit());

    if(!Ce0)
      return;

    clang::NamedDecl *D = dyn_cast_or_null<clang::NamedDecl>(Ce0->getCalleeDecl());
    if(!D)
      return;

    std::string Name = getQnameForSystemSafeDb(D);
    if(!isWaitForAllName(Name))
      return;

    for(auto Each : Ce0->arguments()) {

      Expr *A = Each->IgnoreImplicit();

      if(clang::DeclRefExpr *Dre = getStdMoveArg(A)) {
        clang::ValueDecl *D = Dre->getDecl();
        if(!DeclWhiteList.erase(D)) {
          diag(Dre->getExprLoc(), "(S9.1) awaitable variable not allowed here");
        }
      }
      else if(clang::CallExpr *Ce = dyn_cast<clang::CallExpr>(A)) {
        if(AddCallsToWhiteList)
          CallWhiteList.insert(Ce);
      } else if(clang::DeclRefExpr *Dre = dyn_cast<clang::DeclRefExpr>(A)) {
        clang::ValueDecl *D = Dre->getDecl();
        if(!DeclWhiteList.erase(D)) {
          diag(Dre->getExprLoc(), "(S9.1) awaitable variable not allowed here");
        }
      }
      else {
        diag(Each->getExprLoc(), "(S9.1) nodecpp::wait_for_all argument not allowed");
      }
    }
  }

  void checkCoawaitExpr(clang::CoawaitExpr *AwaitE, bool AddCallsToWhiteList) {

    //first white list this co_await, as is in a valid place
    CoawaitWhiteList.insert(AwaitE);
    
    // then check the argument
    clang::Expr *Ch = AwaitE->getOperand()->IgnoreImplicit();
    if(clang::CallExpr *CallE = dyn_cast<clang::CallExpr>(Ch)) {

      if(AddCallsToWhiteList)
        CallWhiteList.insert(CallE);

      checkWaitForAllCall(CallE, AddCallsToWhiteList);
      //this is a valid call, but don't whitelist, because
      //is under the umbrella of white listed co_await above
      ;
    }
    else if(clang::DeclRefExpr *Dre = dyn_cast<clang::DeclRefExpr>(Ch)) {

      // we must verify this is a valid Decl to co_await, and 
      // in such case mark it as already used.

      clang::ValueDecl *D = Dre->getDecl();
      if(!DeclWhiteList.erase(D)) {
        diag(Ch->getExprLoc(), "(S9.1) not a valid expression to co_await");
        Ch->dumpColor();
      }
    }
    else {
      diag(Ch->getExprLoc(), "(S9.1) not a valid expression to co_await");
      Ch->dumpColor();
    }
  }

  bool VisitStmt(clang::Stmt *St) {

    //TODO update to ValueStmt

    // expression statement is an statement that is an expression
    // and whose direct parent is an statement that is not an expression
    if(clang::Expr *E = dyn_cast<clang::Expr>(St)) {

      const clang::Stmt * P = getParentStmt(Context.getASTContext(), St);
      if(P && !isa<const clang::Expr>(P)) {

        E = E->IgnoreImplicit();
        if(clang::CoawaitExpr *Ce = dyn_cast<clang::CoawaitExpr>(E)) {

          checkCoawaitExpr(Ce, false);
        }
      }
    }
 
    return Super::VisitStmt(St);
  }

  bool VisitDeclStmt(clang::DeclStmt *St) {

    // declaration statement
    if(!St->isSingleDecl()) {
      return Super::VisitDeclStmt(St);
    }

    clang::VarDecl *D = dyn_cast<clang::VarDecl>(St->getSingleDecl());
    if(!D) {
      return Super::VisitDeclStmt(St);
    }

    if(!D->hasInit()) {
      return Super::VisitDeclStmt(St);
    }

    clang::Expr *E = D->getInit()->IgnoreImplicit();

    //TODO if this is a Coawait, then check as stmt like coawait
    if(clang::CoawaitExpr *Ae = dyn_cast<clang::CoawaitExpr>(E)) {

      checkCoawaitExpr(Ae, false);
    }
    else if(clang::CallExpr *Ce = dyn_cast<clang::CallExpr>(E)) {
      
      // if is a call to function returning awaitable, then whitelist
      // the call and the decl var 
      QualType Qt = Ce->getType();
      if(isAwaitableType(Qt)) {
        CallWhiteList.insert(Ce);
        DeclWhiteList.insert(D);
      }
    }
 
    return Super::VisitDeclStmt(St);
  }

  bool VisitCallExpr(clang::CallExpr *E) {

    QualType Qt = E->getType();
    if(isAwaitableType(Qt)) {
      // diag(E->getExprLoc(), "(S9.1) awaitable call"); 
      // E->dumpColor();
      if(!CallWhiteList.erase(E)) {
        diag(E->getExprLoc(), "(S9.1) awaitable object must be awaited");
      }
    }

    return Super::VisitCallExpr(E);
  }

  bool VisitDeclRefExpr(clang::DeclRefExpr *E) {

    QualType Qt = E->getType();
    if(isAwaitableType(Qt)) {
      clang::ValueDecl *D = E->getDecl();
      if(!DeclWhiteList.erase(D)) {
        diag(E->getExprLoc(), "(S9.1) awaitable variable not allowed here");
      }
    }

    return Super::VisitDeclRefExpr(E);
  }

  bool VisitCoawaitExpr(clang::CoawaitExpr *E) {

    //we only support a limited set of uses of co_await
    //and they have all been already checked.
    if(!CoawaitWhiteList.erase(E)) {
      diag(E->getExprLoc(), "(S9.1) co_await not allowed here");
    }

    //don't traverse
    DontTraverseCoawaitedExpr.insert(E->getOperand());
    return Super::VisitCoawaitExpr(E);
  }
  

};

class RuleS91ASTConsumer : public clang::ASTConsumer {

  RuleS91ASTVisitor Visitor;

public:
  RuleS91ASTConsumer(ClangTidyContext &Context) :Visitor(Context) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    Visitor.ReportAndClear();
  }

};

} // namespace checker
} // namespace nodecpp

#endif // NODECPP_CHECKER_RULES91_H

