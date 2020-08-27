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

#include "MustCoAwaitRule.h"
#include "CheckerASTVisitor.h"
#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTLambda.h"

namespace nodecpp {
namespace checker {


class RuleS9ASTVisitor
  : public SafetyASTVisitor<RuleS9ASTVisitor> {

  llvm::SmallSet<CallExpr*, 4> CallWhiteList;
  llvm::SmallSet<CoawaitExpr*, 4> CoawaitWhiteList;
  llvm::SmallSet<ValueDecl*, 4> DeclWhiteList;

  /// \brief keep \c Stmt that have already been checked and shouldn't be traversed 
  llvm::SmallSet<Stmt*, 4> DontTraverse;
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
    return Context->diag(DiagMsgSrc, Loc, Message, Level);
  }

public:

  explicit RuleS9ASTVisitor(ClangTidyContext *Context):
    SafetyASTVisitor<RuleS9ASTVisitor>(Context) {}

  void ReportAndClear() {
    for(auto each : DeclWhiteList) {
      diag(each->getLocation(), "(S9) awaitable variable must be co_awaited");
    }
    DeclWhiteList.clear();
    
    for(auto each : CallWhiteList) {
      each->dump();
      diag(each->getExprLoc(), "(S9) internal check failed");
    }
    CallWhiteList.clear();
    
    for(auto each : CoawaitWhiteList) {
      each->dump();
      diag(each->getExprLoc(), "(S9) internal check failed");
    }
    CoawaitWhiteList.clear();
    
    for(auto each : DontTraverse) {
      each->dump();
      diag(each->getBeginLoc(), "(S9) internal check failed");
    }
    DontTraverse.clear();
  }

  bool TraverseStmt(Stmt *St) {

    if(DontTraverse.erase(St))
      return true;
    else
      return Super::TraverseStmt(St);
  }

  void checkWaitForAllCall(Expr *E, bool AddCallsToWhiteList) {

    CallExpr *Ce0 = dyn_cast<CallExpr>(E->IgnoreImplicit());

    if(!Ce0)
      return;

    NamedDecl *D = dyn_cast_or_null<NamedDecl>(Ce0->getCalleeDecl());
    if(!D)
      return;

    std::string Name = getQnameForSystemSafeDb(D);
    if(!isWaitForAllName(Name))
      return;

    for(auto Each : Ce0->arguments()) {

      Expr *A = Each->IgnoreImplicit();

      if(DeclRefExpr *Dre = getStdMoveArg(A)) {
        ValueDecl *D = Dre->getDecl();
        if(!DeclWhiteList.erase(D)) {
          diag(Dre->getExprLoc(), "(S9) awaitable variable not allowed here");
        }
      }
      else if(CallExpr *Ce = dyn_cast<CallExpr>(A)) {
        if(AddCallsToWhiteList)
          CallWhiteList.insert(Ce);
      } else if(DeclRefExpr *Dre = dyn_cast<DeclRefExpr>(A)) {
        ValueDecl *D = Dre->getDecl();
        if(!DeclWhiteList.erase(D)) {
          diag(Dre->getExprLoc(), "(S9) awaitable variable not allowed here");
        }
      }
      else {
        diag(Each->getExprLoc(), "(S9) wait_for_all argument not allowed");
      }
    }
  }

  void checkCoawaitExpr(CoawaitExpr *AwaitE, bool AddCallsToWhiteList) {

    //first white list this co_await, as is in a valid place
    CoawaitWhiteList.insert(AwaitE);
    
    // then check the argument
    Expr *Ch = AwaitE->getOperand()->IgnoreImplicit();
    if(CallExpr *CallE = dyn_cast<CallExpr>(Ch)) {

      if(AddCallsToWhiteList)
        CallWhiteList.insert(CallE);

      checkWaitForAllCall(CallE, AddCallsToWhiteList);
      //this is a valid call, but don't whitelist, because
      //is under the umbrella of white listed co_await above
      ;
    }
    else if(DeclRefExpr *Dre = dyn_cast<DeclRefExpr>(Ch)) {

      // we must verify this is a valid Decl to co_await, and 
      // in such case mark it as already used.

      ValueDecl *D = Dre->getDecl();
      if(!DeclWhiteList.erase(D)) {
        diag(Ch->getExprLoc(), "(S9) not a valid expression to co_await");
        Ch->dumpColor();
      }
    }
    else {
      diag(Ch->getExprLoc(), "(S9) not a valid expression to co_await");
      Ch->dumpColor();
    }
  }

  bool VisitStmt(Stmt *St) {

    //TODO update to ValueStmt

    // expression statement is an statement that is an expression
    // and whose direct parent is an statement that is not an expression
    if(Expr *E = dyn_cast<Expr>(St)) {

      const Stmt * P = getParentStmt(Context->getASTContext(), St);
      if(P && !isa<const Expr>(P)) {

        E = E->IgnoreImplicit();
        if(CoawaitExpr *Ce = dyn_cast<CoawaitExpr>(E)) {

          checkCoawaitExpr(Ce, false);
        }
      }
    }
 
    return Super::VisitStmt(St);
  }

  bool VisitDeclStmt(DeclStmt *St) {

    // declaration statement
    if(!St->isSingleDecl()) {
      return Super::VisitDeclStmt(St);
    }

    VarDecl *D = dyn_cast<VarDecl>(St->getSingleDecl());
    if(!D) {
      return Super::VisitDeclStmt(St);
    }

    if(!D->hasInit()) {
      return Super::VisitDeclStmt(St);
    }

    Expr *E = D->getInit()->IgnoreImplicit();

    //TODO if this is a Coawait, then check as stmt like coawait
    if(CoawaitExpr *Ae = dyn_cast<CoawaitExpr>(E)) {

      checkCoawaitExpr(Ae, false);
    }
    else if(CallExpr *Ce = dyn_cast<CallExpr>(E)) {
      
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

  bool VisitCallExpr(CallExpr *E) {

    QualType Qt = E->getType();
    if(isAwaitableType(Qt)) {
      // diag(E->getExprLoc(), "(S9) awaitable call"); 
      // E->dumpColor();
      Decl *D = E->getCalleeDecl();
      if(D && D->hasAttr<NodeCppNoAwaitAttr>()) {
        return Super::VisitCallExpr(E);
      }
      else if(D && D->hasAttr<SafeMemoryNoAwaitAttr>()) {
        return Super::VisitCallExpr(E);
      }
      else if(!CallWhiteList.erase(E)) {
        diag(E->getExprLoc(), "(S9) awaitable expression must be co_awaited");
      }
    }

    return Super::VisitCallExpr(E);
  }

  bool VisitDeclRefExpr(DeclRefExpr *E) {

    QualType Qt = E->getType();
    if(isAwaitableType(Qt)) {
      ValueDecl *D = E->getDecl();
      if(!DeclWhiteList.erase(D)) {
        diag(E->getExprLoc(), "(S9) awaitable variable not allowed here");
      }
    }

    return Super::VisitDeclRefExpr(E);
  }

  bool VisitCoawaitExpr(CoawaitExpr *E) {

    //we only support a limited set of uses of co_await
    //and they have all been already checked.
    if(!CoawaitWhiteList.erase(E)) {
      diag(E->getExprLoc(), "(S9) co_await not allowed here");
    }

    //don't traverse
    DontTraverse.insert(E->getOperand());
    return Super::VisitCoawaitExpr(E);
  }
};

class RuleS9ASTConsumer : public ASTConsumer {

  RuleS9ASTVisitor Visitor;

public:
  RuleS9ASTConsumer(ClangTidyContext *Context) :Visitor(Context) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    Visitor.ReportAndClear();
  }

};

std::unique_ptr<clang::ASTConsumer> makeMustCoAwaitRule(ClangTidyContext *Context) {
  return llvm::make_unique<RuleS9ASTConsumer>(Context);
}

} // namespace checker
} // namespace nodecpp

