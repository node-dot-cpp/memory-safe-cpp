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

#define DEBUG_LOGGING 0

static bool isTrackedVar(const VarDecl *vd, const DeclContext *dc) {
  if (vd->isLocalVarDecl() && !vd->hasGlobalStorage() &&
      !vd->isExceptionVariable() && !vd->isInitCapture() &&
      !vd->isImplicit() && vd->getDeclContext() == dc) {
    QualType ty = vd->getType();
    return ty->isScalarType() || ty->isVectorType() || ty->isRecordType();
  }
  return false;
}

//------------------------------------------------------------------------====//
// DeclToIndex: a mapping from Decls we track to value indices.
//====------------------------------------------------------------------------//

namespace {

class DeclToIndex {
  llvm::DenseMap<const VarDecl *, unsigned> map;

public:
  DeclToIndex() = default;

  /// Compute the actual mapping from declarations to bits.
  void computeMap(const DeclContext &dc);

  /// Return the number of declarations in the map.
  unsigned size() const { return map.size(); }

  /// Returns the bit vector index for a given declaration.
  Optional<unsigned> getValueIndex(const VarDecl *d) const;
};

} // namespace

void DeclToIndex::computeMap(const DeclContext &dc) {
  unsigned count = 0;
  DeclContext::specific_decl_iterator<VarDecl> I(dc.decls_begin()),
                                               E(dc.decls_end());
  for ( ; I != E; ++I) {
    const VarDecl *vd = *I;
    if (isTrackedVar(vd, &dc))
      map[vd] = count++;
  }
}

Optional<unsigned> DeclToIndex::getValueIndex(const VarDecl *d) const {
  llvm::DenseMap<const VarDecl *, unsigned>::const_iterator I = map.find(d);
  if (I == map.end())
    return None;
  return I->second;
}

//------------------------------------------------------------------------====//
// CFGBlockValues: dataflow values for CFG blocks.
//====------------------------------------------------------------------------//

// These values are defined in such a way that a merge can be done using
// a bitwise OR.
enum Value { Unknown = 0x0,         /* 00 */
             Initialized = 0x1,     /* 01 */
             Uninitialized = 0x2,   /* 10 */
             MayUninitialized = 0x3,/* 11 */
             MayBeZoombie = 0x4    /* 100 */ };

static bool isUninitialized(const Value v) {
  return v >= Uninitialized;
}

static bool isAlwaysUninit(const Value v) {
  return v == Uninitialized;
}

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

      if(hasDecl(Vd))
        return false;

      VariablesToDZ.push_back(Vd);
      return true;
    }

    bool addThis() {
      bool Tmp = ThisToDZ;
      ThisToDZ = true;
      return !Tmp;
    }

    void clear() {
      VariablesToDZ.clear();
      ThisToDZ = false;
      IsInitialized = false;
    }

    bool intersection(const Scratch& other) {

      if(!IsInitialized) {
        *this = other;
        IsInitialized = true;
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



  const CFG &cfg;
  SmallVector<Scratch, 8> Vals;
//  Scratch scratch;
  DeclToIndex declToIndex;

public:
  CFGBlockValues(const CFG &cfg) : cfg(cfg), Vals(cfg.getNumBlockIDs()) {}

  unsigned getNumEntries() const { return declToIndex.size(); }

  // void computeSetOfDeclarations(const DeclContext &dc);

  const Scratch &getScratch(unsigned int Id) {
    return Vals[Id];
  }

  // void setAllScratchValues(Value V);
  // void mergeIntoScratch(ValueVector const &source, bool isFirst);
   bool updateWithScratch(unsigned int Id, const Scratch& Scr) {
      return Vals[Id].intersection(Scr);
    }

  // bool hasNoDeclarations() const {
  //   return declToIndex.size() == 0;
  // }

  // void resetScratch() {
  //   scratch.clear();
  // }

  // // trys to add a VarDecl, returns true if success,
  // // returns false if was already here 
  // bool tryAddToScratch(const ValueDecl *Vd) {
  //   return scratch.addDecl(Vd);
  // }
};

} // namespace

// void CFGBlockValues::computeSetOfDeclarations(const DeclContext &dc) {
//   declToIndex.computeMap(dc);
//   unsigned decls = declToIndex.size();
//   scratch.resize(decls);
//   unsigned n = cfg.getNumBlockIDs();
//   if (!n)
//     return;
//   vals.resize(n);
//   for (auto &val : vals)
//     val.resize(decls);
// }

// #if DEBUG_LOGGING
// static void printVector(const CFGBlock *block, ValueVector &bv,
//                         unsigned num) {
//   llvm::errs() << block->getBlockID() << " :";
//   for (const auto &i : bv)
//     llvm::errs() << ' ' << i;
//   llvm::errs() << " : " << num << '\n';
// }
// #endif

// void CFGBlockValues::setAllScratchValues(Value V) {
//   for (unsigned I = 0, E = scratch.size(); I != E; ++I)
//     scratch[I] = V;
// }

// void CFGBlockValues::mergeIntoScratch(ValueVector const &source,
//                                       bool isFirst) {
//   if (isFirst)
//     scratch = source;
//   else
//     scratch |= source;
// }

// bool CFGBlockValues::updateValueVectorWithScratch(const CFGBlock *block) {
//   ValueVector &dst = getValueVector(block);
//   bool changed = (dst != scratch);
//   if (changed)
//     dst = scratch;
// #if DEBUG_LOGGING
//   printVector(block, scratch, 0);
// #endif
//   return changed;
// }


// ValueVector::reference CFGBlockValues::operator[](const VarDecl *vd) {
//   const Optional<unsigned> &idx = declToIndex.getValueIndex(vd);
//   assert(idx.hasValue());
//   return scratch[idx.getValue()];
// }

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

namespace {

class FindVarResult {
  const VarDecl *vd;
  const DeclRefExpr *dr;

public:
  FindVarResult(const VarDecl *vd, const DeclRefExpr *dr) : vd(vd), dr(dr) {}

  const DeclRefExpr *getDeclRefExpr() const { return dr; }
  const VarDecl *getDecl() const { return vd; }
};

} // namespace

static const Expr *stripCasts(ASTContext &C, const Expr *Ex) {
  while (Ex) {
    Ex = Ex->IgnoreParenNoopCasts(C);
    if (const auto *CE = dyn_cast<CastExpr>(Ex)) {
      if (CE->getCastKind() == CK_LValueBitCast) {
        Ex = CE->getSubExpr();
        continue;
      }
    }
    break;
  }
  return Ex;
}

/// If E is an expression comprising a reference to a single variable, find that
/// variable.
static FindVarResult findVar(const Expr *E, const DeclContext *DC) {
  if (const auto *DRE =
          dyn_cast<DeclRefExpr>(stripCasts(DC->getParentASTContext(), E)))
    if (const auto *VD = dyn_cast<VarDecl>(DRE->getDecl()))
      if (isTrackedVar(VD, DC))
        return FindVarResult(VD, DRE);
  return FindVarResult(nullptr, nullptr);
}

namespace {

/// Classify each DeclRefExpr as an initialization or a use. Any
/// DeclRefExpr which isn't explicitly classified will be assumed to have
/// escaped the analysis and will be treated as an initialization.
class ClassifyRefs : public StmtVisitor<ClassifyRefs> {
public:
  enum Class {
    Init,
    Use,
    SelfInit,
    Ignore
  };

private:
  const DeclContext *DC;
  llvm::DenseMap<const DeclRefExpr *, Class> Classification;

  bool isTrackedVar(const VarDecl *VD) const {
    return ::isTrackedVar(VD, DC);
  }

  void classify(const Expr *E, Class C);

public:
  ClassifyRefs(AnalysisDeclContext &AC) : DC(cast<DeclContext>(AC.getDecl())) {}

  void VisitDeclStmt(DeclStmt *DS);
  void VisitUnaryOperator(UnaryOperator *UO);
  void VisitBinaryOperator(BinaryOperator *BO);
  void VisitCallExpr(CallExpr *CE);
  void VisitCastExpr(CastExpr *CE);

  void operator()(Stmt *S) { Visit(S); }

  Class get(const DeclRefExpr *DRE) const {
    llvm::DenseMap<const DeclRefExpr*, Class>::const_iterator I
        = Classification.find(DRE);
    if (I != Classification.end())
      return I->second;

    const auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
    if (!VD || !isTrackedVar(VD))
      return Ignore;

    return Init;
  }
};

} // namespace

static const DeclRefExpr *getSelfInitExpr(VarDecl *VD) {
  if (VD->getType()->isRecordType())
    return nullptr;
  if (Expr *Init = VD->getInit()) {
    const auto *DRE =
        dyn_cast<DeclRefExpr>(stripCasts(VD->getASTContext(), Init));
    if (DRE && DRE->getDecl() == VD)
      return DRE;
  }
  return nullptr;
}

void ClassifyRefs::classify(const Expr *E, Class C) {
  // The result of a ?: could also be an lvalue.
  E = E->IgnoreParens();
  if (const auto *CO = dyn_cast<ConditionalOperator>(E)) {
    classify(CO->getTrueExpr(), C);
    classify(CO->getFalseExpr(), C);
    return;
  }

  if (const auto *BCO = dyn_cast<BinaryConditionalOperator>(E)) {
    classify(BCO->getFalseExpr(), C);
    return;
  }

  if (const auto *OVE = dyn_cast<OpaqueValueExpr>(E)) {
    classify(OVE->getSourceExpr(), C);
    return;
  }

  if (const auto *ME = dyn_cast<MemberExpr>(E)) {
    if (const auto *VD = dyn_cast<VarDecl>(ME->getMemberDecl())) {
      if (!VD->isStaticDataMember())
        classify(ME->getBase(), C);
    }
    return;
  }

  if (const auto *BO = dyn_cast<BinaryOperator>(E)) {
    switch (BO->getOpcode()) {
    case BO_PtrMemD:
    case BO_PtrMemI:
      classify(BO->getLHS(), C);
      return;
    case BO_Comma:
      classify(BO->getRHS(), C);
      return;
    default:
      return;
    }
  }

  FindVarResult Var = findVar(E, DC);
  if (const DeclRefExpr *DRE = Var.getDeclRefExpr())
    Classification[DRE] = std::max(Classification[DRE], C);
}

void ClassifyRefs::VisitDeclStmt(DeclStmt *DS) {
  for (auto *DI : DS->decls()) {
    auto *VD = dyn_cast<VarDecl>(DI);
    if (VD && isTrackedVar(VD))
      if (const DeclRefExpr *DRE = getSelfInitExpr(VD))
        Classification[DRE] = SelfInit;
  }
}

void ClassifyRefs::VisitBinaryOperator(BinaryOperator *BO) {
  // Ignore the evaluation of a DeclRefExpr on the LHS of an assignment. If this
  // is not a compound-assignment, we will treat it as initializing the variable
  // when TransferFunctions visits it. A compound-assignment does not affect
  // whether a variable is uninitialized, and there's no point counting it as a
  // use.
  if (BO->isCompoundAssignmentOp())
    classify(BO->getLHS(), Use);
  else if (BO->getOpcode() == BO_Assign || BO->getOpcode() == BO_Comma)
    classify(BO->getLHS(), Ignore);
}

void ClassifyRefs::VisitUnaryOperator(UnaryOperator *UO) {
  // Increment and decrement are uses despite there being no lvalue-to-rvalue
  // conversion.
  if (UO->isIncrementDecrementOp())
    classify(UO->getSubExpr(), Use);
}

static bool isPointerToConst(const QualType &QT) {
  return QT->isAnyPointerType() && QT->getPointeeType().isConstQualified();
}

void ClassifyRefs::VisitCallExpr(CallExpr *CE) {
  // Classify arguments to std::move as used.
  if (CE->isCallToStdMove()) {
    // RecordTypes are handled in SemaDeclCXX.cpp.
    if (!CE->getArg(0)->getType()->isRecordType())
      classify(CE->getArg(0), Use);
    return;
  }

  // If a value is passed by const pointer or by const reference to a function,
  // we should not assume that it is initialized by the call, and we
  // conservatively do not assume that it is used.
  for (CallExpr::arg_iterator I = CE->arg_begin(), E = CE->arg_end();
       I != E; ++I) {
    if ((*I)->isGLValue()) {
      if ((*I)->getType().isConstQualified())
        classify((*I), Ignore);
    } else if (isPointerToConst((*I)->getType())) {
      const Expr *Ex = stripCasts(DC->getParentASTContext(), *I);
      const auto *UO = dyn_cast<UnaryOperator>(Ex);
      if (UO && UO->getOpcode() == UO_AddrOf)
        Ex = UO->getSubExpr();
      classify(Ex, Ignore);
    }
  }
}

void ClassifyRefs::VisitCastExpr(CastExpr *CE) {
  if (CE->getCastKind() == CK_LValueToRValue)
    classify(CE->getSubExpr(), Use);
  else if (const auto *CSE = dyn_cast<CStyleCastExpr>(CE)) {
    if (CSE->getType()->isVoidType()) {
      // Squelch any detected load of an uninitialized value if
      // we cast it to void.
      // e.g. (void) x;
      classify(CSE->getSubExpr(), Ignore);
    }
  }
}

//------------------------------------------------------------------------====//
// Transfer function for uninitialized values analysis.
//====------------------------------------------------------------------------//

namespace {

class ScratchCalculator : public StmtVisitor<ScratchCalculator> {
  
  Scratch& InOut;

public:
  ScratchCalculator(Scratch& InOut): InOut(InOut) {}

  void VisitCallExpr(CallExpr *Ce) {
    if (Decl *Callee = Ce->getCalleeDecl()) {
      InOut.clear();
    }
  }

  void VisitDeclRefExpr(DeclRefExpr *Dre) {
    if(Dre->isDezombiefyCandidate()) {
      if(!InOut.addDecl(Dre->getDecl())) {
        //it was already there
        Dre->setDezombiefyCandidateButRelaxed();
      }
    }
  }

  void VisitCXXThisExpr(CXXThisExpr *E) {
    if(E->isDezombiefyCandidate()) {
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


//------------------------------------------------------------------------====//
// High-level "driver" logic for uninitialized values analysis.
//====------------------------------------------------------------------------//

static void runOnBlock(const CFGBlock *block,
                       Scratch &InOut) {

  ScratchCalculator Sc(InOut);
  for (const auto &I : *block) {
    if (Optional<CFGStmt> cs = I.getAs<CFGStmt>())

      Sc.Visit(const_cast<Stmt *>(cs->getStmt()));
  }
}


void nodecpp::runDezombiefyRelaxAnalysis(
    const DeclContext &dc,
    const CFG &cfg,
    AnalysisDeclContext &ac,
    UninitVariablesHandler &handler,
    UninitVariablesAnalysisStats &stats) {
  CFGBlockValues vals(cfg);
  // vals.computeSetOfDeclarations(dc);
  // if (vals.hasNoDeclarations())
  //   return;

  stats.NumVariablesAnalyzed = vals.getNumEntries();

  // Precompute which expressions are uses and which are initializations.
  ClassifyRefs classification(ac);
  cfg.VisitBlockStmts(classification);

  // // Mark all variables uninitialized at the entry.
  // const CFGBlock &entry = cfg.getEntry();
  // ValueVector &vec = vals.getValueVector(&entry);
  // const unsigned n = vals.getNumEntries();
  // for (unsigned j = 0; j < n; ++j) {
  //   vec[j] = Uninitialized;
  // }

  // Proceed with the workist.
  Worklist Wl(cfg.getNumBlockIDs());
  llvm::BitVector previouslyVisited(cfg.getNumBlockIDs());
  Wl.enqueue(&cfg.getEntry());
  llvm::BitVector wasAnalyzed(cfg.getNumBlockIDs(), false);
  wasAnalyzed[cfg.getEntry().getBlockID()] = true;
//  PruneBlocksHandler PBH(cfg.getNumBlockIDs());

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

  //   PBH.currentBlock =

  //   // Did the block change?
  //   bool changed = runOnBlock(block, cfg, ac, vals,
  //                             classification, wasAnalyzed, PBH);
  //   ++stats.NumBlockVisits;
  //   if (changed || !previouslyVisited[block->getBlockID()])
  //     worklist.enqueueSuccessors(block);
  //   previouslyVisited[block->getBlockID()] = true;
  // }

  // if (!PBH.hadAnyUse)
  //   return;

  // Run through the blocks one more time, and report uninitialized variables.
//   for (const auto *Block : cfg) {
// //    if (PBH.hadUse[block->getBlockID()]) {
//   }

//       ++stats.NumBlockVisits;
//    }
}

UninitVariablesHandler::~UninitVariablesHandler() = default;
