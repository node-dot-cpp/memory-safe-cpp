/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
* All rights reserved.
*
* Initial version copied from lib/Analysis/UninitializedValues.cpp
*
* -------------------------------------------------------------------------------*/
//===- UninitializedValues.cpp - Find Uninitialized Values ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements uninitialized values analysis for source-level CFGs.
//
//===----------------------------------------------------------------------===//

#include "DezombiefyRelaxAnalysis.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtObjC.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Analysis/Analyses/PostOrderCFGView.h"
#include "clang/Analysis/AnalysisDeclContext.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/DomainSpecific/ObjCNoReturn.h"
#include "clang/Basic/LLVM.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/PackedVector.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <cassert>

using namespace clang;
using namespace nodecpp;


namespace {

  struct Scratch {
    //TODO change to Set 
    llvm::SmallVector<const clang::ValueDecl*, 4> VariablesToDZ;
    bool ThisToDZ = false;
    bool IsInitialized = false;

    Scratch() {}
    Scratch(const Scratch&) = default;
    
    explicit Scratch(const clang::ValueDecl* Dc) :VariablesToDZ({Dc}) {}
    explicit Scratch(bool Th) :ThisToDZ(Th) {}

    bool hasDecl(const clang::ValueDecl *Vd) const {
      for(auto Each : VariablesToDZ) {
        if(Each == Vd) //already here
          return true;
      }

      return false;
    }

    bool addDecl(const clang::ValueDecl *Vd) {

      IsInitialized = true;
      if(hasDecl(Vd))
        return false;

      VariablesToDZ.push_back(Vd);
      return true;
    }

    bool addThis() {
      IsInitialized = true;

      bool Tmp = ThisToDZ;
      ThisToDZ = true;
      return !Tmp;
    }

    void clear() {
      VariablesToDZ.clear();
      ThisToDZ = false;
      IsInitialized = true;
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

      llvm::SmallVector<const clang::ValueDecl*, 4> Tmp;
      for(auto Each : other.VariablesToDZ) {
        if(hasDecl(Each))
          Tmp.push_back(Each);
      }

      Changed = Changed || VariablesToDZ.size() != Tmp.size();
      VariablesToDZ = std::move(Tmp);

      return Changed;
    }
  };


class CFGBlockValues {
  SmallVector<Scratch, 8> Vals;

public:
  CFGBlockValues(unsigned int NumBlockIDs) : Vals(NumBlockIDs) {}

  const Scratch &getScratch(unsigned int Id) {
    return Vals[Id];
  }

  bool updateWithScratch(unsigned int Id, const Scratch& Scr) {
    return Vals[Id].intersection(Scr);
  }
};

} // namespace


//------------------------------------------------------------------------====//
// Worklist: worklist for dataflow analysis.
//====------------------------------------------------------------------------//

namespace {

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

} // namespace


//------------------------------------------------------------------------====//
// Classification of DeclRefExprs as use or initialization.
//====------------------------------------------------------------------------//

// namespace {

// class FindVarResult {
//   const VarDecl *vd;
//   const DeclRefExpr *dr;

// public:
//   FindVarResult(const VarDecl *vd, const DeclRefExpr *dr) : vd(vd), dr(dr) {}

//   const DeclRefExpr *getDeclRefExpr() const { return dr; }
//   const VarDecl *getDecl() const { return vd; }
// };

// } // namespace

// static const Expr *stripCasts(ASTContext &C, const Expr *Ex) {
//   while (Ex) {
//     Ex = Ex->IgnoreParenNoopCasts(C);
//     if (const auto *CE = dyn_cast<CastExpr>(Ex)) {
//       if (CE->getCastKind() == CK_LValueBitCast) {
//         Ex = CE->getSubExpr();
//         continue;
//       }
//     }
//     break;
//   }
//   return Ex;
// }

// /// If E is an expression comprising a reference to a single variable, find that
// /// variable.
// static FindVarResult findVar(const Expr *E, const DeclContext *DC) {
//   if (const auto *DRE =
//           dyn_cast<DeclRefExpr>(stripCasts(DC->getParentASTContext(), E)))
//     if (const auto *VD = dyn_cast<VarDecl>(DRE->getDecl()))
//       if (isTrackedVar(VD, DC))
//         return FindVarResult(VD, DRE);
//   return FindVarResult(nullptr, nullptr);
// }


//------------------------------------------------------------------------====//
// ScratchCalculator
//====------------------------------------------------------------------------//

namespace {

class ScratchCalculator : public StmtVisitor<ScratchCalculator> {
  
  Scratch& InOut;

public:
  ScratchCalculator(Scratch& InOut): InOut(InOut) {}

  void VisitCallExpr(CallExpr *Ce) {
//    Ce->dumpColor();
    if (Decl *Callee = Ce->getCalleeDecl()) {
      InOut.clear();
    }
  }

  void VisitDeclRefExpr(DeclRefExpr *Dre) {
    if(Dre->isDezombiefyCandidateOrRelaxed()) {
      if(InOut.addDecl(Dre->getDecl())) {
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

} // namespace


static void runOnBlock(const CFGBlock *block,
                       Scratch &InOut) {

  ScratchCalculator Sc(InOut);
  for (const auto &I : *block) {
    if (Optional<CFGStmt> cs = I.getAs<CFGStmt>()) {

      Sc.Visit(const_cast<Stmt *>(cs->getStmt()));
//      cs->getStmt()->dumpColor();
    }
  }
}


void nodecpp::runDezombiefyRelaxAnalysis(
    const DeclContext *dc,
    const CFG *cfg,
    DezombiefyRelaxAnalysisStats &stats) {

  CFGBlockValues vals(cfg->getNumBlockIDs());
  // Proceed with the workist.
  Worklist Wl(cfg->getNumBlockIDs());
  Wl.enqueue(&cfg->getEntry());

  while (const CFGBlock *Block = Wl.dequeue()) {
    
    auto Id =  Block->getBlockID();
    Scratch InOut = vals.getScratch(Id);//make a copy
    runOnBlock(Block, InOut);

    ++stats.NumBlockVisits;

    //Update all successors
    for(auto S : Block->succs()) {
      if(S.isReachable()) {
        bool Changed = vals.updateWithScratch(S->getBlockID(), InOut);
        if(Changed) {
          Wl.enqueue(S.getReachableBlock());
        }
      }
    }
  }
}


void nodecpp::runDezombiefyRelaxAnalysis(const clang::FunctionDecl *D) {

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

    runDezombiefyRelaxAnalysis(D, cfg, stats);

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
