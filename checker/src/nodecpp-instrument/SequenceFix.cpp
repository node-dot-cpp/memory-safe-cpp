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

#include "SequenceFix.h"
#include "BaseASTVisitor.h"
#include "CodeChange.h"
#include "DezombiefyHelper.h"
#include "SequenceCheckASTVisitor.h"
#include "SequenceFixASTVisitor.h"
#include "UnwrapperASTVisitor.h"

#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Sema/Sema.h"

namespace nodecpp {

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;


class SequenceCheck2ASTVisitor
  : public BaseASTVisitor<SequenceCheck2ASTVisitor> {

  bool DebugReportMode = false;

  ZombieIssuesStats Stats;


  bool needExtraBraces(const Stmt *St) {

    auto SList = Context.getParents(*St);

    auto SIt = SList.begin();

    if (SIt == SList.end())
      return true;

    return SIt->get<CompoundStmt>() == nullptr;
  }

  bool isIfCondVarDeclStmt(DeclStmt  *St) {

    auto SList = Context.getParents(*St);

    auto SIt = SList.begin();

    if (SIt == SList.end())
      return false;

    if(auto P = SIt->get<IfStmt>())
      return P->getConditionVariableDeclStmt() == St;
    else
      return false;
  }

  bool isIfCondExpr(Expr *E) {

    auto SList = Context.getParents(*E);

    auto SIt = SList.begin();

    if (SIt == SList.end())
      return false;

    if(auto P = SIt->get<IfStmt>())
      return P->getCond() == E;
    else
      return false;
  }


  bool isStandAloneStmt(Stmt *St) {

    auto SList = Context.getParents(*St);

    auto SIt = SList.begin();

    if (SIt == SList.end())
      return false;

    if(SIt->get<CompoundStmt>())
      return true;
    else if(auto P = SIt->get<IfStmt>())
      return P->getThen() == St || P->getElse() == St;
    else if(auto P = SIt->get<WhileStmt>())
      return P->getBody() == St;
    else if(auto P = SIt->get<ForStmt>())
      return P->getBody() == St;
    else if(auto P = SIt->get<DoStmt>())
      return P->getBody() == St;
    else
      return false;      
  }

  bool callCheckSequence(clang::Expr *E, ZombieSequence ZqMax) {
    return checkSequence(Context, E, ZqMax, DebugReportMode, SilentMode, Stats);
  }

public:
  explicit SequenceCheck2ASTVisitor(ASTContext &Context, bool DebugReportMode, bool SilentMode):
    BaseASTVisitor<SequenceCheck2ASTVisitor>(Context, SilentMode),
     DebugReportMode(DebugReportMode) {}

  auto getStats() {
    return Stats;
  }


  void tryFixExpr(const Stmt *St, Expr *E) {

    if(E) {
      bool Fix = callCheckSequence(E, ZombieSequence::Z2);
      if(Fix) {
        FileChanges R;
        bool Ok = applyUnwrapFix(Context, SilentMode, R, Index, St, E);
        if(Ok) {
          addReplacement(R);
        }
      }
    }
  }

  void tryFixDeclStmt(const Stmt *Parent, DeclStmt *St) {

    if(St->isSingleDecl()) {
      if(VarDecl *D = dyn_cast_or_null<VarDecl>(St->getSingleDecl())) {
        tryFixExpr(Parent, D->getInit());
      }
    }
    else {
      if(!SilentMode)
        llvm::errs() << "Multi decl not supported by zombie analysis (yet)\n";
    }
  }

  bool TraverseStmt(Stmt *St) {
    // For every root expr, sent it to check and don't traverse it here
    if(!St)
      return true;
    else if(Expr *E = dyn_cast<Expr>(St)) {
      if(isStandAloneStmt(St)) {
        bool Fix = callCheckSequence(E, ZombieSequence::Z2);
        if(Fix) {
          FileChanges R;
          bool Ok = applyUnwrapFix(Context, SilentMode, R, Index, E);
          if(Ok) {
            addReplacement(R);
          }
        }
      }
      else if(isIfCondExpr(E)) {
        auto SList = Context.getParents(*St);
        auto SIt = SList.begin();

        assert (SIt != SList.end());
        auto P = SIt->get<IfStmt>();

        tryFixExpr(P, E);
        return true;
      }
      else {
        bool Fix = callCheckSequence(E, ZombieSequence::Z1);
        if(Fix) {
          FileChanges R;
          bool Ok = applyOp2CallFix(Context, SilentMode, R, E);
          if(Ok) {

            addReplacement(R);

          }
        }
      }

      return true;
    }
    else
      return Base::TraverseStmt(St);
  }

  bool TraverseDeclStmt(DeclStmt *St) {
    
    if(isStandAloneStmt(St)) {
      tryFixDeclStmt(St, St);
      return true;
    }
    else if(isIfCondVarDeclStmt(St)) {
      auto SList = Context.getParents(*St);
      auto SIt = SList.begin();

      assert (SIt != SList.end());
      auto P = SIt->get<IfStmt>();

      tryFixDeclStmt(P, St);
      return true;
    }
    else
      return Base::TraverseDeclStmt(St);
  }
};

void ZombieIssuesStats::printStats() {
  
  llvm::errs() << "Issues stats Z1:" << Z1Count << ", Z2:" <<
    Z2Count << ", Z9:" << Z9Count << "\n";
  llvm::errs() << "Fix stats Op2Call:" << Op2CallFixCount << ", Unwrap:" <<
    UnwrapFixCount << "\n";
  llvm::errs() << "Unfixed stats Z2:" << UnfixedZ2Count << ", Z9:" <<
    UnfixedZ9Count << "\n";
}

void sequenceFix(ASTContext &Ctx,  bool DebugReportMode, bool SilentMode) {

  SequenceCheck2ASTVisitor V1(Ctx, DebugReportMode, SilentMode);

  V1.TraverseDecl(Ctx.getTranslationUnitDecl());

  V1.getStats().printStats();
  
  if(DebugReportMode)
    return;

  auto &Reps = V1.finishReplacements();
  overwriteChangedFiles(Ctx, Reps, "nodecpp-sequence-fix");
}


} //namespace nodecpp