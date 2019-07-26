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

#include "DezombiefyHelper.h"

#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Sema/Sema.h"

namespace nodecpp {

using namespace clang;

/// Visitor for expressions which looks for unsequenced access to potencially
/// zombie objects and calls to functions that may zombie them as side-effects
/// When this two things happend in an unsequenced region, dezombiefy can't
/// be used reliably.
class SequenceChecker : public EvaluatedExprVisitor<SequenceChecker> {
  using Base = EvaluatedExprVisitor<SequenceChecker>;

  /// A tree of sequenced regions within an expression. Two regions are
  /// unsequenced if one is an ancestor or a descendent of the other. When we
  /// finish processing an expression with sequencing, such as a comma
  /// expression, we fold its tree nodes into its parent, since they are
  /// unsequenced with respect to nodes we will visit later.
  class SequenceTree {
    struct Value {
      explicit Value(unsigned Parent) : Parent(Parent), Merged(false) {}
      unsigned Parent : 31;
      unsigned Merged : 1;
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
    };

    SequenceTree() { Values.push_back(Value(0)); }
    Seq root() const { return Seq(0); }

    /// Create a new sequence of operations, which is an unsequenced
    /// subset of \p Parent. This sequence of operations is sequenced with
    /// respect to other children of \p Parent.
    Seq allocate(Seq Parent) {
      Values.push_back(Value(Parent.Index));
      return Seq(Values.size() - 1);
    }

    /// Merge a sequence of operations into its parent.
    void merge(Seq S) {
      Values[S.Index].Merged = true;
    }

    /// Determine whether two operations are unsequenced. This operation
    /// is asymmetric: \p Cur should be the more recent sequence, and \p Old
    /// should have been merged into its parent as appropriate.
    bool isUnsequenced(Seq Cur, Seq Old) {
      unsigned C = representative(Cur.Index);
      unsigned Target = representative(Old.Index);
      while (C >= Target) {
        if (C == Target)
          return true;
        C = Values[C].Parent;
      }
      return false;
    }

  private:
    /// Pick a representative for a sequence.
    unsigned representative(unsigned K) {
      if (Values[K].Merged)
        // Perform path compression as we go.
        return Values[K].Parent = representative(Values[K].Parent);
      return K;
    }
  };

    class RegionRaii {
        SequenceTree& Tree;
        SequenceTree::Seq& Current;
        SequenceTree::Seq Parent;
        SmallVector<SequenceTree::Seq, 32> Elts;
    public:
        RegionRaii(SequenceTree& Tree, SequenceTree::Seq& Region) 
        :Tree(Tree), Current(Region), Parent(Region) {}

        void beginNewRegion() {
            Current = Tree.allocate(Parent);
            Elts.push_back(Current);
        }

        void restoreParentRegion() {
            Current = Parent;
        }

        ~RegionRaii() {
            restoreParentRegion(); //just in case
            for (unsigned I = 0; I < Elts.size(); ++I)
              Tree.merge(Elts[I]);
        }
    };

  llvm::SmallDenseMap<Expr *, SequenceTree::Seq, 16> UsageInfoMapThisOrRef;
  llvm::SmallDenseMap<Expr *, SequenceTree::Seq, 16> UsageInfoMapCall;

  ASTContext &Context;

  /// Sequenced regions within the expression.
  SequenceTree Tree;

  /// The region we are currently within.
  SequenceTree::Seq Region;

  /// Should we report diagnose, or this is only a pre-check
  bool ReportDiagnostics = false;

  /// Did we find any issues
  bool IssuesFound = false;


    enum EvaluatedCond {False = false, True = true, Unknown};
    EvaluatedCond evaluate(const Expr *E) const {
        bool R;
      bool Ok = E->EvaluateAsBooleanCondition(R, Context);
        return Ok ? static_cast<EvaluatedCond>(R) : Unknown;
    }

  void notePotencialZombie(Expr *E) {
    UsageInfoMapThisOrRef[E] = Region;

    for(auto& Each : UsageInfoMapCall)
      if(Tree.isUnsequenced(Region, Each.second)) {
        //report error
        diag(E, Each.first);
      }
  }

  /// a call can rezombiefy objects in unsequenced zones
  void noteCall(Expr *E) {
    UsageInfoMapCall[E] = Region;

    for(auto& Each : UsageInfoMapThisOrRef)
      if(Tree.isUnsequenced(Region, Each.second)) {
        //report error
        diag(Each.first, E);
      }
  }

  void diag(Expr *E1, Expr *E2) {
      IssuesFound = true;
    if(ReportDiagnostics) {
        diag("Dezombiefy", E1->getExprLoc(), 
        "Potencially zombie object and potencial zombie creator call are unsequenced, dezombiefication will not be realiable", 
        DiagnosticIDs::Error) << E2->getExprLoc();
    }
  }

  DiagnosticBuilder diag(
        StringRef CheckName, SourceLocation Loc, StringRef Description,
        DiagnosticIDs::Level Level /* = DiagnosticIDs::Warning*/) {
    //  assert(Loc.isValid());
    unsigned ID = Context.getDiagnostics().getDiagnosticIDs()->getCustomDiagID(
        Level, (Description + " [" + CheckName + "]").str());
    // CheckNamesByDiagnosticID.try_emplace(ID, CheckName);
    return Context.getDiagnostics().Report(Loc, ID);
  }

  public:
  SequenceChecker(ASTContext &Context, bool ReportDiagnostics)
      : Base(Context), Context(Context), Region(Tree.root()), ReportDiagnostics(ReportDiagnostics) {
  }

    bool foundIssues() const { return IssuesFound; }

  void VisitStmt(Stmt *S) {
    // Skip all statements which aren't expressions for now.
  }

  void VisitExpr(Expr *E) {
    // By default, just recurse to evaluated subexpressions.
    Base::VisitStmt(E);
  }


  void VisitCXXThisExpr(CXXThisExpr *E) {
    if(!E->isImplicit()) {
      notePotencialZombie(E);
      return;
    }

    //debug
    const Expr *P = getParentExpr(Context, E);
    if(!P || !isa<MemberExpr>(P))
        P->dumpColor();
  }

  void VisitMemberExpr(MemberExpr *E) {
    if(CXXThisExpr *Te = dyn_cast_or_null<CXXThisExpr>(E->getBase())) {
      if(Te->isImplicit()) {
          notePotencialZombie(E);
          return;
      }
    }
    Base::VisitMemberExpr(E);
  }

  void VisitDeclRefExpr(DeclRefExpr *E) {
    if(IsDezombiefyCandidate(E))
      notePotencialZombie(E);
  }

//   void VisitCastExpr(CastExpr *E) {
//     Object O = Object();
//     if (E->getCastKind() == CastKind::CK_LValueToRValue)
//       O = getObject(E->getSubExpr(), false);

//     if (O)
//       notePreUse(O, E);
//     VisitExpr(E);
//     if (O)
//       notePostUse(O, E);
//   }

  void VisitBinComma(BinaryOperator *BO) {
    // C++11 [expr.comma]p1:
    //   Every value computation and side effect associated with the left
    //   expression is sequenced before every value computation and side
    //   effect associated with the right expression.

    RegionRaii RR(Tree, Region);

    RR.beginNewRegion();
    Visit(BO->getLHS());

    RR.beginNewRegion();
    Visit(BO->getRHS());

  }

  void VisitBinAssign(BinaryOperator *E) {
      //C++17
    // In every simple assignment expression E1=E2 and
    // every compound assignment expression E1@=E2,
    // every value computation and side-effect of E2
    // is sequenced before every value computation and side effect of E1
    RegionRaii RR(Tree, Region);

    RR.beginNewRegion();
    Visit(E->getLHS());

    RR.beginNewRegion();
    Visit(E->getRHS());
  }

  void VisitCompoundAssignOperator(CompoundAssignOperator *CAO) {
    VisitBinAssign(CAO);
  }

//   void VisitUnaryPreInc(UnaryOperator *UO) { VisitUnaryPreIncDec(UO); }
//   void VisitUnaryPreDec(UnaryOperator *UO) { VisitUnaryPreIncDec(UO); }
//   void VisitUnaryPreIncDec(UnaryOperator *UO) {
//     Object O = getObject(UO->getSubExpr(), true);
//     if (!O)
//       return VisitExpr(UO);

//     notePreMod(O, UO);
//     Visit(UO->getSubExpr());
//     // C++11 [expr.pre.incr]p1:
//     //   the expression ++x is equivalent to x+=1
//     notePostMod(O, UO, UK_ModAsValue);
//   }

//   void VisitUnaryPostInc(UnaryOperator *UO) { VisitUnaryPostIncDec(UO); }
//   void VisitUnaryPostDec(UnaryOperator *UO) { VisitUnaryPostIncDec(UO); }
//   void VisitUnaryPostIncDec(UnaryOperator *UO) {
//     Object O = getObject(UO->getSubExpr(), true);
//     if (!O)
//       return VisitExpr(UO);

//     notePreMod(O, UO);
//     Visit(UO->getSubExpr());
//     notePostMod(O, UO, UK_ModAsSideEffect);
//   }

  /// Don't visit the RHS of '&&' or '||' if it might not be evaluated.
  void VisitBinLOr(BinaryOperator *E) {
    // The side-effects of the LHS of an '&&' are sequenced before the
    // value computation of the RHS, and hence before the value computation
    // of the '&&' itself, unless the LHS evaluates to zero. We treat them
    // as if they were unconditionally sequenced.
    RegionRaii RR(Tree, Region);

    RR.beginNewRegion();
    Visit(E->getLHS());


    // if it will short-circuit, don't go into RHS
    auto R = evaluate(E->getLHS());
    if(R == True)
        return;

    RR.beginNewRegion();
    Visit(E->getRHS());
  }
  void VisitBinLAnd(BinaryOperator *E) {
          
          
    RegionRaii RR(Tree, Region);
    
    RR.beginNewRegion();
    Visit(E->getLHS());

    // if it will short-circuit, don't go into RHS
     auto R = evaluate(E->getLHS());
      if (R == False)
        return;

    RR.beginNewRegion();
    Visit(E->getRHS());
  }

  void VisitAbstractConditionalOperator(AbstractConditionalOperator *E) {

    RegionRaii RR(Tree, Region);

    RR.beginNewRegion();
    Visit(E->getCond());

    // if we know only one branck will be evaluated, don't visit the other
     auto R = evaluate(E->getCond());

    if(R == True || R == Unknown) {
        RR.beginNewRegion();
        Visit(E->getTrueExpr());
    }

    if(R == False || R == Unknown) {
        RR.beginNewRegion();
        Visit(E->getFalseExpr());

    }
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
    
    RegionRaii RR(Tree, Region);

    //call body is sequenced respect to its own arguments
    RR.beginNewRegion();
    noteCall(E);

    if(E->getCallee()) {
        RR.beginNewRegion();
        Visit(E->getCallee());
    }

    for (auto Begin = E->arg_begin(), End = E->arg_end();
         Begin != End; ++Begin) {
        RR.beginNewRegion();
        Visit(*Begin);
    }
  }

  void VisitCXXConstructExpr(CXXConstructExpr *E) {

    // In C++11, list initializations are sequenced.
    // In C++17, other constructor as function call

    RegionRaii RR(Tree, Region);

    //call body is sequenced respect to its own arguments
    RR.beginNewRegion();
    noteCall(E);

    for (auto Begin = E->arg_begin(), End = E->arg_end();
         Begin != End; ++Begin) {
        RR.beginNewRegion();
        Visit(*Begin);
    }
  }

  void VisitInitListExpr(InitListExpr *E) {

    // In C++11, list initializations are sequenced.
    RegionRaii RR(Tree, Region);
    for (auto Begin = E->begin(), End = E->end();
         Begin != End; ++Begin) {
        RR.beginNewRegion();
        Visit(*Begin);
    }
  }
};


bool checkUnsequencedDezombiefy(ASTContext &Context, Expr *E, bool ReportDiagnostics) {
//  E->dumpColor();
    SequenceChecker V(Context, ReportDiagnostics);
    V.Visit(E);
    return V.foundIssues();
}

} //namespace nodecpp