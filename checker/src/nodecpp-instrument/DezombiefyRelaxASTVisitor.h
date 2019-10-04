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

#ifndef NODECPP_CHECKER_DEZOMBIEFYRELAXASTVISITOR_H
#define NODECPP_CHECKER_DEZOMBIEFYRELAXASTVISITOR_H

#include "DezombiefyHelper.h"

#include "BaseASTVisitor.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/AnalysisDeclContext.h"


namespace nodecpp {

using namespace clang;
using namespace llvm;

struct DezombiefyRelaxAnalysisStats {
//  unsigned NumVariablesAnalyzed = 0;
  unsigned NumBlockVisits = 0;
};

struct Scratch {

  DenseSet<const VarDecl*> VariablesToDZ;
  bool ThisToDZ = false;

  // we need to know if a clear was ever called on this Scratch
  // or if is empty because it was never modified
  bool IsInitialized = false;

  Scratch() {}
  Scratch(const Scratch&) = default;
  
  // explicit Scratch(const clang::VarDecl* Dc) :VariablesToDZ({Dc}) {}
  // explicit Scratch(bool Th) :ThisToDZ(Th) {}

  bool hasDecl(const VarDecl *Vd) const {

    return VariablesToDZ.find(Vd) != VariablesToDZ.end();
  }

  bool addDecl(const VarDecl *Vd) {

    IsInitialized = true;
    return VariablesToDZ.insert(Vd).second;
  }

  bool addThis() {
    IsInitialized = true;

    bool Tmp = ThisToDZ;
    ThisToDZ = true;
    return !Tmp;
  }

  void clear() {
    IsInitialized = true;
    VariablesToDZ.clear();
    ThisToDZ = false;
  }

  bool intersection(const Scratch& other) {

    if(!other.IsInitialized) {
      return false;
    }
    else if(!IsInitialized) {
      *this = other;
//        IsInitialized = true;
      return true;
    }



    bool Changed = ThisToDZ && !other.ThisToDZ;
    ThisToDZ = ThisToDZ && other.ThisToDZ;

    DenseSet<const VarDecl*> Tmp;
    for(auto Each : other.VariablesToDZ) {
      if(VariablesToDZ.find(Each) != VariablesToDZ.end()) {
        Tmp.insert(Each);
      }
    }

    Changed = Changed || VariablesToDZ.size() != Tmp.size();
    VariablesToDZ = std::move(Tmp);

    return Changed;
  }
};


// FakeCFGBlockValues don't really track things, but simplifies the algo
// so every block is independant and is assument to begin without any
// already dezombiefied variable
class FakeCFGBlockValues {
std::vector<bool> Vals;

public:
FakeCFGBlockValues(unsigned int NumBlockIDs) : Vals(NumBlockIDs, false) {}

Scratch copyScratchAtInput(unsigned int Id) {
  Vals[Id] = true;
  return Scratch();
}

bool updateWithOutputScratch(unsigned int Id, const Scratch& Scr) {
  return !Vals[Id];
}
};


// CFGBlockValues keep a list of things that are already dezombiefied
// when we go into a Block, when a Block is reachable from more than
// one other Block, then we need to make the minimal common between both

class CFGBlockValues {
std::vector<Scratch> Vals;

public:
CFGBlockValues(unsigned int NumBlockIDs) : Vals(NumBlockIDs) {}

Scratch copyScratchAtInput(unsigned int Id) {
  return Vals[Id];
}

bool updateWithOutputScratch(unsigned int Id, const Scratch& Scr) {
  return Vals[Id].intersection(Scr);
}
};

//------------------------------------------------------------------------====//
// Worklist: worklist for dataflow analysis.
//====------------------------------------------------------------------------//

class Worklist {
//  PostOrderCFGView::iterator PO_I, PO_E;
  SmallVector<const CFGBlock *, 20> worklist;
  llvm::BitVector enqueuedBlocks;

public:
  Worklist(unsigned long NumBlockIDs) :
        enqueuedBlocks(NumBlockIDs, false) {}

  void enqueue(const CFGBlock *Block) {
    if (!Block || enqueuedBlocks[Block->getBlockID()])
      return;

    worklist.push_back(Block);
    enqueuedBlocks[Block->getBlockID()] = true;
  }

  const CFGBlock *dequeue() {
    if (worklist.empty())
      return nullptr;

    auto B = worklist.pop_back_val();
    assert(enqueuedBlocks[B->getBlockID()] == true);
    enqueuedBlocks[B->getBlockID()] = false;
    return B;
  }
};



//------------------------------------------------------------------------====//
// ScratchCalculator
//====------------------------------------------------------------------------//

class ScratchCalculator : public StmtVisitor<ScratchCalculator> {
  
  Scratch& InOut;

public:
  ScratchCalculator(Scratch& InOut): InOut(InOut) {}

  void VisitDeclStmt(DeclStmt *St) {
    
    if(St->isSingleDecl()) {
      VarDecl *D = dyn_cast_or_null<VarDecl>(St->getSingleDecl());
      if(D && mayZombie(D->getType()) && D->getInit()) {
        InOut.addDecl(D);
      }
    }
  }

  void VisitCallExpr(CallExpr *Ce) {
//    Ce->dumpColor();
    if (Decl *Callee = Ce->getCalleeDecl()) {
      InOut.clear();
    }
  }

  void VisitDeclRefExpr(DeclRefExpr *Dre) {
    if(Dre->isDezombiefyCandidateOrRelaxed()) {
      VarDecl *D = dyn_cast_or_null<VarDecl>(Dre->getDecl());
      if(InOut.addDecl(D)) {
        //It may be relaxed by previous path,
        // but need to make it explicit now
        Dre->setDezombiefyCandidate();
      }
      else {
        //it was already there
        Dre->setDezombiefyCandidateButRelaxed();
      }
    }
  }

  void VisitCXXThisExpr(CXXThisExpr *E) {
    if(E->isDezombiefyCandidateOrRelaxed()) {
      if(InOut.addThis()) {
        //It may be relaxed by previous path,
        // but need to make it explicit now
        E->setDezombiefyCandidate();
      } else {
        //it was already there
        E->setDezombiefyCandidateButRelaxed();
      }
    }
  }
};

static
void runOnBlock(const CFGBlock *block, Scratch &InOut) {

  ScratchCalculator Sc(InOut);
  for (const auto &I : *block) {
    if (Optional<CFGStmt> cs = I.getAs<CFGStmt>()) {

      Sc.Visit(const_cast<Stmt *>(cs->getStmt()));
    }
  }
}

template<class ScratchManager>
void runDezombiefyRelaxAnalysis(
    const DeclContext *dc,
    const CFG *cfg,
    DezombiefyRelaxAnalysisStats &stats) {

  ScratchManager vals(cfg->getNumBlockIDs());
  // Proceed with the workist.
  Worklist Wl(cfg->getNumBlockIDs());
  Wl.enqueue(&cfg->getEntry());

  while (const CFGBlock *Block = Wl.dequeue()) {

    auto Id =  Block->getBlockID();
    Scratch InOut = vals.copyScratchAtInput(Id);//make  a copy
    runOnBlock(Block, InOut);

    ++stats.NumBlockVisits;

    //Update all successors
    for(auto S : Block->succs()) {
      if(S.isReachable()) {
        bool Changed = vals.updateWithOutputScratch(S->getBlockID(), InOut);
        if(Changed) {
          Wl.enqueue(S.getReachableBlock());
        }
      }
    }
  }
}

void runDezombiefyRelaxAnalysis(const clang::FunctionDecl *D) {

  // Construct the analysis context with the specified CFG build options.
  AnalysisDeclContext AC(/* AnalysisDeclContextManager */ nullptr, D);

  // Don't generate EH edges for CallExprs as we'd like to avoid the n^2
  // explosion for destructors that can result and the compile time hit.
  AC.getCFGBuildOptions().PruneTriviallyFalseEdges = true;
  AC.getCFGBuildOptions().AddEHEdges = false;
  AC.getCFGBuildOptions().AddInitializers = true;
  AC.getCFGBuildOptions().AddImplicitDtors = true;
  AC.getCFGBuildOptions().AddTemporaryDtors = true;
  AC.getCFGBuildOptions().AddCXXNewAllocator = false;
  AC.getCFGBuildOptions().AddCXXDefaultInitExprInCtors = true;

  AC.getCFGBuildOptions().setAllAlwaysAdd();

  if (CFG *cfg = AC.getCFG()) {
    DezombiefyRelaxAnalysisStats stats;

//    cfg->dump(Context.getLangOpts(), true);

//    runDezombiefyRelaxAnalysis<CFGBlockValues>(D, cfg, stats);
    runDezombiefyRelaxAnalysis<FakeCFGBlockValues>(D, cfg, stats);

    // if (S.CollectStats && stats.NumVariablesAnalyzed > 0) {
    //   ++NumUninitAnalysisFunctions;
    //   NumUninitAnalysisVariables += stats.NumVariablesAnalyzed;
    //   NumUninitAnalysisBlockVisits += stats.NumBlockVisits;
    //   MaxUninitAnalysisVariablesPerFunction =
    //       std::max(MaxUninitAnalysisVariablesPerFunction,
    //                 stats.NumVariablesAnalyzed);
    //   MaxUninitAnalysisBlockVisitsPerFunction =
    //       std::max(MaxUninitAnalysisBlockVisitsPerFunction,
    //                 stats.NumBlockVisits);
    // }
  }
}





class DezombiefyRelaxASTVisitor
  : public BaseASTVisitor<DezombiefyRelaxASTVisitor> {

  using Base = BaseASTVisitor<DezombiefyRelaxASTVisitor>;

public:
  explicit DezombiefyRelaxASTVisitor(clang::ASTContext &Context):
    Base(Context) {}

  bool VisitFunctionDecl(clang::FunctionDecl *D) {

    // For code in dependent contexts, we'll do this at instantiation time.
    if (D->isDependentContext())
      return true;

    if(!D->getBody())
      return true;

    runDezombiefyRelaxAnalysis(D);

    return Base::VisitFunctionDecl(D);
  }
};

} // namespace nodecpp

#endif // NODECPP_CHECKER_DEZOMBIEFYRELAXASTVISITOR_H

