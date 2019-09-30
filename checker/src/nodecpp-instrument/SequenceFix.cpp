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

  bool DontFix = false;
  bool FixAll = false;
  int Index = 0;

  bool needExtraBraces(Stmt *St) {

    auto SList = Context.getParents(*St);

    auto SIt = SList.begin();

    if (SIt == SList.end())
      return true;

    return SIt->get<CompoundStmt>() == nullptr;
  }

  bool isStandAloneExpr(Stmt *St) {

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

public:
  explicit SequenceCheck2ASTVisitor(ASTContext &Context, bool FixAll, bool DontFix):
    BaseASTVisitor<SequenceCheck2ASTVisitor>(Context),
     FixAll(FixAll), DontFix(DontFix) {}

  bool TraverseStmt(Stmt *St) {
    // For every root expr, sent it to check and don't traverse it here
    if(!St)
      return true;
    else if(Expr *E = dyn_cast<Expr>(St)) {
      if(isStandAloneExpr(St)) {
        auto P = checkSequence(Context, E, ZombieSequence::Z2, DontFix);
        if(!DontFix && P != ZombieSequence::NONE) {
          ExpressionUnwrapperVisitor V2(Context, Index);
          auto &R = V2.unwrapExpression(St, E, true, true);
          addReplacement(R);
        }
      }
      else {
        auto P = checkSequence(Context, E, ZombieSequence::Z1, DontFix);
        if(!DontFix && P != ZombieSequence::NONE) {
          SequenceFixASTVisitor V2(Context);
          auto &R = V2.fixExpression(E);
          addReplacement(R);
        }
      }

      return true;
    }
    else
      return Base::TraverseStmt(St);
  }

  bool TraverseDeclStmt(DeclStmt *St) {
    
    if(St->isSingleDecl()) {
      if(VarDecl *D = dyn_cast_or_null<VarDecl>(St->getSingleDecl())) {
        if(Expr *E = D->getInit()) {
          auto P = checkSequence(Context, E, ZombieSequence::Z2, DontFix);
          if(!DontFix && P != ZombieSequence::NONE) {
            ExpressionUnwrapperVisitor V2(Context, Index);
            auto &R = V2.unwrapExpression(St, E, needExtraBraces(St), false);
            addReplacement(R);
          }

          return true;
        }
      }
    }
    return Base::TraverseDeclStmt(St);
  }

};



void sequenceFix(ASTContext &Ctx, bool ReportOnlyDontFix) {
      
  SequenceCheck2ASTVisitor V1(Ctx, false, ReportOnlyDontFix);

  V1.TraverseDecl(Ctx.getTranslationUnitDecl());

  if(ReportOnlyDontFix)
    return;

  auto &Reps = V1.finishReplacements();
  overwriteChangedFiles(Ctx, Reps, "nodecpp-unsequenced");
}


} //namespace nodecpp