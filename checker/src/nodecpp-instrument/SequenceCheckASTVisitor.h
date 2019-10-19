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

#ifndef NODECPP_CHECKER_SEQUENCECHECKASTVISITOR_H
#define NODECPP_CHECKER_SEQUENCECHECKASTVISITOR_H

#include "SequenceFix.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Format/Format.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"

namespace nodecpp {

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;


enum class ZombieSequence {NONE, Z1, Z2, Z9};

class ZombieIssues {
  std::vector<std::pair<Expr *, Expr *>> Z1Issues;
  std::vector<std::pair<Expr *, Expr *>> Z2Issues;
  std::vector<std::pair<Expr *, Expr *>> Z9Issues;

public:
  void addStats(ZombieIssuesStats& Stats) const {
    Stats.Z1Count += Z1Issues.size();
    Stats.Z2Count += Z2Issues.size();
    Stats.Z9Count += Z9Issues.size();
  }

  void addIssue(ZombieSequence Zs, Expr *E1, Expr *E2) {
    if(Zs == ZombieSequence::Z1)
      Z1Issues.emplace_back(E1, E2);
    else if(Zs == ZombieSequence::Z2)
      Z2Issues.emplace_back(E1, E2);
    else if(Zs == ZombieSequence::Z9)
      Z9Issues.emplace_back(E1, E2);
  }

  ZombieSequence getMaxIssue() const {
    if(!Z9Issues.empty())
      return ZombieSequence::Z9;
    else if(!Z2Issues.empty())
      return ZombieSequence::Z2;
    else if(!Z1Issues.empty())
      return ZombieSequence::Z1;
    else
      return ZombieSequence::NONE;
  }

  static
  void reportIssue(DiagnosticsEngine &De, StringRef CheckName,
    SourceLocation Loc1, SourceLocation Loc2, 
    StringRef ZombieSeq, StringRef Description,
    DiagnosticIDs::Level Level) {

    unsigned ID = De.getDiagnosticIDs()->getCustomDiagID(
        Level, ("(" + ZombieSeq + ") " + Description + " [" + CheckName + "]").str());
    // CheckNamesByDiagnosticID.try_emplace(ID, CheckName);
    De.Report(Loc1, ID) << Loc2;

  }

  static
  void reportIssue(DiagnosticsEngine &De, StringRef ZombieSeq,
    const Expr *E1, const Expr *E2) {
    reportIssue(De, "nodecpp-dezombiefy", E1->getExprLoc(), E2->getExprLoc(),
      ZombieSeq, "Dezombiefication not fully realiable", DiagnosticIDs::Error);
    // E1->dump();
    // E2->dumpColor();
  }

  void reportIssue(DiagnosticsEngine &De) const {
    if(!Z9Issues.empty())
      reportIssue(De, "Z9",
        Z9Issues.front().first, Z9Issues.front().second);
    if(!Z2Issues.empty())
      reportIssue(De, "Z2",
        Z2Issues.front().first, Z2Issues.front().second);
    else if(!Z1Issues.empty())
      reportIssue(De, "Z1",
        Z1Issues.front().first, Z1Issues.front().second);
  }

  void reportAllIssues(DiagnosticsEngine &De) const {
    for(auto &Each : Z9Issues)
      reportIssue(De, "Z9", Each.first, Each.second);
    for(auto &Each : Z2Issues)
      reportIssue(De, "Z2", Each.first, Each.second);
    for(auto &Each : Z1Issues)
      reportIssue(De, "Z1", Each.first, Each.second);
  }
};

/// Visitor for expressions which looks for unsequenced access to potencially
/// zombie objects and calls to functions that may zombie them as side-effects
/// When this two things happend in an unsequenced region, dezombiefy can't
/// be used reliably.
class SequenceCheckASTVisitor2 : public EvaluatedExprVisitor<SequenceCheckASTVisitor2> {
  using Base = EvaluatedExprVisitor<SequenceCheckASTVisitor2>;

  class SequenceTree {
    struct Value {
      explicit Value(unsigned Parent, bool Sequenced) : Parent(Parent), Sequenced(Sequenced) {}
      unsigned Parent : 31;
      unsigned Sequenced : 1;
    };
    SmallVector<Value, 8> Values;

  public:
    /// A region within an expression which may be sequenced with respect
    /// to some other region.
    class Seq {
      friend class SequenceTree;

      unsigned Index = 0;

      explicit Seq(unsigned N) : Index(N) {}

    public:
      Seq() = default;
      bool operator==(Seq O) const { return Index == O.Index; }
      bool operator!=(Seq O) const { return !this->operator==(O); }
    };

    SequenceTree() { Values.push_back(Value(0, false)); }
    Seq root() const { return Seq(0); }

    /// Create a new sequence of operations, which is an unsequenced
    /// subset of \p Parent. This sequence of operations is sequenced with
    /// respect to other children of \p Parent.
    Seq allocate(Seq Parent, bool Sequenced) {
      Values.push_back(Value(Parent.Index, Sequenced));
      return Seq(Values.size() - 1);
    }

    unsigned commonAncestor(Seq Left, Seq Right) {
      unsigned L = Left.Index;
      unsigned R = Right.Index;
      while (L != R) {
        if (L < R)
          R = Values[R].Parent;
        else
          L = Values[L].Parent;
      }
      return L;
    }

    bool isZ1(Seq Cur, Seq Old) {
      //this should return true if the common parent
      // is an unsequenced node.
      return  !(Values[commonAncestor(Cur, Old)].Sequenced);
    }

    bool isZ2(Seq Cur, Seq Old) {
      //this should return true if Old is a parent or Cur
      return  commonAncestor(Cur, Old) == Old.Index;
    }

    Seq beginUnsequensed(Seq Cur) { return allocate(Cur, false); }
    Seq beginSequensed(Seq Cur) { return allocate(Cur, true); }
  };

  class RegionRaii {
    SequenceTree& Tree;
    SequenceTree::Seq& Current;
    SequenceTree::Seq Parent;

  public:
    RegionRaii(SequenceTree& Tree, SequenceTree::Seq& Region, bool Sequenced) 
    :Tree(Tree), Current(Region), Parent(Region) {
      Current = Tree.allocate(Parent, Sequenced);
    }

    ~RegionRaii() {
      Current = Parent;
    }
  };


  llvm::SmallDenseMap<Expr *, SequenceTree::Seq, 16> Z1Refs;
  llvm::SmallDenseMap<Expr *, SequenceTree::Seq, 16> ZCalls;
  llvm::SmallDenseMap<Expr *, SequenceTree::Seq, 16> Z2Refs;
  

  ASTContext &Context; //non-const, as base visitor has a const ref

  /// Sequenced regions within the expression.
  SequenceTree Tree;

  /// The region we are currently within.
  SequenceTree::Seq Region;

  /// The kind of issue we need to fix for this expression
  ZombieIssues Issues;

  RegionRaii beginSequenced() {
    return RegionRaii(Tree, Region, true);
  }

  RegionRaii beginUnsequenced() {
    return RegionRaii(Tree, Region, false);
  }

  void noteZ2Ref(Expr *E) {
    Z2Refs[E] = Region;
  }

  void removeZ2Ref(Expr *E) {
    Z2Refs.erase(E);
  }

  void noteZ1Ref(Expr *E) {
    Z1Refs[E] = Region;

    for(auto& Each : ZCalls) {
      if(Tree.isZ1(Each.second, Region)) {
        addIssue(ZombieSequence::Z1,  E, Each.first);
      }
    }
  }

  /// a call can rezombiefy objects in unsequenced zones
  void noteCall(Expr *E) {
    ZCalls[E] = Region;

    for(auto& Each : Z1Refs) {
      if(Tree.isZ1(Region, Each.second)) {
        addIssue(ZombieSequence::Z1, Each.first, E);
      }
    }
    for(auto& Each : Z2Refs) {
      if(Tree.isZ2(Region, Each.second)) {
        addIssue(ZombieSequence::Z2, Each.first, E);
      }
    }
  }

  void addIssue(ZombieSequence Zs, Expr *E1, Expr *E2) {
    Issues.addIssue(Zs, E1, E2);
  }

  public:
  SequenceCheckASTVisitor2(ASTContext &Context)
      : Base(Context), Context(Context), Region(Tree.root()) {
  }

  const ZombieIssues& getIssues() const {
    return Issues;
  }

  void VisitStmt(Stmt *S) {
    // Skip all statements which aren't expressions for now.
  }

  void VisitExpr(Expr *E) {
    // By default, just recurse to evaluated subexpressions.
    Base::VisitStmt(E);
  }


  void VisitCXXThisExpr(CXXThisExpr *E) {

      noteZ1Ref(E);
  }

  void VisitDeclRefExpr(DeclRefExpr *E) {
    if(isDezombiefyCandidate(E))
      noteZ1Ref(E);
  }

  void VisitBinaryOperator(clang::BinaryOperator *E) {
    switch(E->getOpcode()) {
      case BO_Mul:
      case BO_Div:
      case BO_Rem:
      case BO_Add:
      case BO_Sub:
      case BO_LT:
      case BO_GT:
      case BO_LE:
      case BO_GE:
      case BO_EQ:
      case BO_NE:
      case BO_Cmp:
      case BO_And:
      case BO_Xor:
      case BO_Or:
        {
          auto Raii = beginUnsequenced();
          Base::VisitBinaryOperator(E);
        }
        break;
      case BO_PtrMemD:
      case BO_PtrMemI:
      case BO_Shl:
      case BO_Shr:
      case BO_LAnd:
      case BO_LOr :
      case BO_Assign:
      case BO_MulAssign:
      case BO_DivAssign:
      case BO_RemAssign:
      case BO_AddAssign:
      case BO_SubAssign:
      case BO_ShlAssign:
      case BO_ShrAssign:
      case BO_AndAssign:
      case BO_OrAssign:
      case BO_XorAssign:
      case BO_Comma:
        {
          auto Raii = beginSequenced();
          Base::VisitBinaryOperator(E);
        }
        break;
      default:
        assert(false);
    }
  }

  static
  bool calculateOthers(const vector<bool>& Args, unsigned I) {

    for(unsigned J = 0; J != Args.size(); ++J) {
      if(I != J) {
        if(Args[J]) {
          return true;
        } 
      }
    }

    return false;
  }

  bool mayBaseZombie(MemberExpr* E) {
    //we dig MemberExpr that are not pointers until we find a 'this'
    // a pointer access, or something relevant
    auto Be = E->getBase()->IgnoreParenCasts();
    if(isa<CXXThisExpr>(Be)) {
      // we assume 'this' is already not a zombie
      return false;
    }
    else if(E->isArrow()) {
      //any arrow not being 'this' may zombie
      return true;
    }
    else if(auto Me = dyn_cast<MemberExpr>(Be)) {
      // a nested member not arrow needs digging
      return mayBaseZombie(Me);
    }
    else if(auto Dre = dyn_cast<DeclRefExpr>(Be)) {
      // anything resolved to a VarDecl or ParamVarDecl
      // may zombie if ref or ptr to sys type
      if(auto D = dyn_cast_or_null<VarDecl>(Dre->getDecl())) {
        auto Qt = D->getType().getCanonicalType();
        if(Qt->isLValueReferenceType() || Qt->isPointerType()) {
          Qt = Qt->getPointeeType().getCanonicalType();
          if(Qt->isStructureOrClassType()) {
//            Qt.dump();
            auto Rd = Qt->getAsCXXRecordDecl();
            bool SysType = needsExternalDezombiefy(Context, Rd);
            return SysType;
          }
        }
      }
      return false;
    }
    else // anything else assume may zombie
      return true;
  }

  void VisitCallExpr(CallExpr *E) {
    // C++11 [intro.execution]p15:
    //   When calling a function [...], every value computation and side effect
    //   associated with any argument expression, or with the postfix expression
    //   designating the called function, is sequenced before execution of every
    //   expression or statement in the body of the function [and thus before
    //   the value computation of its result].

    // C++17  [expr.call]p5:
    //   The postfix-expression is sequenced before each expression in the
    //   expression-list and any default argument. The initialization of a parameter,
    //   including every associated value computation and side effect, is
    //   indeterminately sequenced with respect to that of any other parameter. 
    
//    RegionRaii RR(Tree, Region);
    auto Raii = beginSequenced();
    //this call is noted in parents region

    bool AtLeastOneArgumentMayZombie = false;
    bool BaseMayZombie = false;
    bool IsSystem = false; 

    auto D = E->getDirectCallee();
    if(D) {
      IsSystem = needsExternalDezombiefy(Context, D->getCanonicalDecl());
    }
    else {
      // this can happend with pointer to member kind of things
      // we can't really know where it is going to land
      // so assume external dezombiefy is required
      IsSystem = true;
    } 

    auto CalleT = E->getCallee()->getType().getCanonicalType().getTypePtrOrNull();
    if(isa<BuiltinType>(CalleT)) {
      llvm::errs() << "isa<BuiltinType>(CalleT)\n";
      CalleT->dump();
      return;
    }

    noteCall(E);

    auto Pt = dyn_cast_or_null<clang::PointerType>(CalleT);
    if(!Pt) {
      llvm::errs() << "VisitCallExpr: !Pt\n";
      E->dumpColor();
      E->getCallee()->getType().getCanonicalType().dump();
      // assert(false);
      return;
    }

    auto Fpt = dyn_cast_or_null<clang::FunctionProtoType>(
      Pt->getPointeeType().getCanonicalType().getTypePtrOrNull());
    if(!Fpt) {
      llvm::errs() << "VisitCallExpr: !Pt\n";
      E->dumpColor();
      Pt->getPointeeType().getCanonicalType().dump();
      // assert(false);
      return;
    }
    
    Expr *Ce = E->getCallee()->IgnoreParenCasts();
    
    //mb: here we need to analyze what is going to be the 
    // 'this' in the called method, to see if we can 
    // verify it will not be a zombie
    if(!Ce) {
      // not really sure this can happend
      E->dumpColor();
      //assert(false);
      return;
    }
    else if(auto Me = dyn_cast<MemberExpr>(Ce) ) {
      
      BaseMayZombie = mayBaseZombie(Me) ;
//      Me->dumpColor();
      
    }
    else if(auto Dre = dyn_cast<DeclRefExpr>(Ce)) {
      //this is a function call or a CXXOperatorExpr
      if(isa<CXXMethodDecl>(Dre->getDecl()))
        BaseMayZombie = true;

    }
    else {
      Ce->dumpColor();
      assert(false);
      return;
    }

    Visit(E->getCallee());

    vector<bool> P;
    unsigned Count = Fpt->getNumParams();
    //TODO on CXXOperatorCallExpr, the callee expr
    // is an argument. then the count mismatch
    unsigned Offset = E->getNumArgs() == Count + 1 ? 1 : 0;
    assert(E->getNumArgs() == Count + Offset);

    // if(Offset == 1) {
    //   E->getArg(0)->IgnoreParenImpCasts()->dumpColor();
    // }

    for(unsigned I = 0; I != Count; ++I) {
      auto EachT = Fpt->getParamType(I);
      auto EachE = E->getArg(I + Offset);

      // for argument to zombie, the type called declaration must be a ref or ptr
      // and it must be initialized from something not being a plain value
      bool Z = mayZombie(EachT);
      if(Z) {
        if(auto Dre = dyn_cast<DeclRefExpr>(EachE->IgnoreParenCasts())) {
          Z = isDezombiefyCandidate(Dre);
        }
      }

      P.push_back(Z);
      if(Z) {
        AtLeastOneArgumentMayZombie = true;
        noteZ1Ref(EachE);
      }
    }

    if(BaseMayZombie || AtLeastOneArgumentMayZombie) {
      //check for issue Z9 
      for(unsigned I = 0; I != Count; ++I) {
        auto EachT = Fpt->getParamType(I);
        if (isByValueUserTypeNonTriviallyCopyable(Context, EachT)) {
          //report a Z9 issue here.
          addIssue(ZombieSequence::Z9, E->getArg(I + Offset), E);

        }
      }
    }

    for(unsigned I = 0; I != Count; ++I) {
      auto Each = E->getArg(I);
      bool B = IsSystem && (BaseMayZombie || calculateOthers(P, I));
      if(B)
        noteZ2Ref(E);

      Visit(Each);

      if(B)
        removeZ2Ref(E);
    }
  }

  // void VisitCXXConstructExpr(CXXConstructExpr *E) {

  //   // In C++11, list initializations are sequenced.
  //   // In C++17, other constructor as function call

  //   RegionRaii RR(Tree, Region);

  //   //call body is sequenced respect to its own arguments
  //   RR.beginNewRegion();
  //   noteCall(E);

  //   for (auto Begin = E->arg_begin(), End = E->arg_end();
  //        Begin != End; ++Begin) {
  //       RR.beginNewRegion();
  //       Visit(*Begin);
  //   }
  // }

  // void VisitInitListExpr(InitListExpr *E) {

  //   // In C++11, list initializations are sequenced.
  //   RegionRaii RR(Tree, Region);
  //   for (auto Begin = E->begin(), End = E->end();
  //        Begin != End; ++Begin) {
  //       RR.beginNewRegion();
  //       Visit(*Begin);
  //   }
  // }
};


bool checkSequence(clang::ASTContext &Context, clang::Expr *E, ZombieSequence ZqMax, bool ReportOnly, ZombieIssuesStats& Stats) {

  // for constant expressions, we assume they are ok
  if(E->isCXX11ConstantExpr(Context))
    return false;

  SequenceCheckASTVisitor2 V(Context);
  V.Visit(E);
  auto &Issues = V.getIssues();
  Issues.addStats(Stats);
  if(Issues.getMaxIssue() == ZombieSequence::NONE) {
    //all ok
    return false;
  }
  else if(ReportOnly) {
    Issues.reportAllIssues(Context.getDiagnostics());
    return false;
  }
  else if(Issues.getMaxIssue() <= ZqMax) {
    //needs fix
    return true;
  }
  else {
    //can't fix, report
    Issues.reportIssue(Context.getDiagnostics());
    return false;
  }
}

} // namespace nodecpp

#endif // NODECPP_CHECKER_SEQUENCECHECKASTVISITOR_H

