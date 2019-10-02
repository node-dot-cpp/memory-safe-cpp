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


enum class ZombieSequence {NONE, Z1, Z2 };

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
  

  ASTContext &Context;

  /// Sequenced regions within the expression.
  SequenceTree Tree;

  /// The region we are currently within.
  SequenceTree::Seq Region;

  /// The kind of issue we need to fix for this expression
  ZombieSequence Issue = ZombieSequence::NONE;

  /// Where we should report a warning because
  /// we won't be able to fix the expression
  ZombieSequence MaxIssue = ZombieSequence::NONE;

  bool ReportAll = false;


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
        if(updateIssue(ZombieSequence::Z1))
          diag("Z1", E, Each.first);
      }
    }
  }

  /// a call can rezombiefy objects in unsequenced zones
  void noteCall(Expr *E) {
    ZCalls[E] = Region;

    for(auto& Each : Z1Refs) {
      if(Tree.isZ1(Region, Each.second)) {
        //report error
        if(updateIssue(ZombieSequence::Z1))
          diag("Z1", Each.first, E);
      }
    }
    for(auto& Each : Z2Refs) {
      if(Tree.isZ2(Region, Each.second)) {
        if(updateIssue(ZombieSequence::Z2))
          diag("Z2", Each.first, E);
      }
    }
  }

  bool updateIssue(ZombieSequence Value) {
    if(Value > Issue) {
      Issue = Value;
      return true;
    }
    return ReportAll;
  }


  void diag(StringRef ZombieSeq, Expr *E1, Expr *E2) {
    if(true) {
        diag("nodecpp-dezombiefy", E1->getExprLoc(), ZombieSeq,
        "Dezombiefication not fully realiable", 
        DiagnosticIDs::Error) << E2->getExprLoc();
    }
  }

  DiagnosticBuilder diag(
        StringRef CheckName, SourceLocation Loc, StringRef ZombieSeq, StringRef Description,
        DiagnosticIDs::Level Level /* = DiagnosticIDs::Warning*/) {
    //  assert(Loc.isValid());
    unsigned ID = Context.getDiagnostics().getDiagnosticIDs()->getCustomDiagID(
        Level, ("(" + ZombieSeq + ") " + Description + " [" + CheckName + "]").str());
    // CheckNamesByDiagnosticID.try_emplace(ID, CheckName);
    return Context.getDiagnostics().Report(Loc, ID);
  }

  public:
  SequenceCheckASTVisitor2(ASTContext &Context, ZombieSequence MaxIssue, bool ReportAll)
      : Base(Context), Context(Context), Region(Tree.root()),
      MaxIssue(MaxIssue), ReportAll(ReportAll) {
  }

  ZombieSequence getIssue() const {
    return Issue;
  }

  void VisitStmt(Stmt *S) {
    // Skip all statements which aren't expressions for now.
  }

  void VisitExpr(Expr *E) {
    // By default, just recurse to evaluated subexpressions.
    Base::VisitStmt(E);
  }


  void VisitCXXThisExpr(CXXThisExpr *E) {
//    if(!E->isImplicit()) {
      noteZ1Ref(E);
      // return;
//    }

    //shouldn't reach here
    // const Expr *P = getParentExpr(Context, E);
    // P->dumpColor();
    // errs() << "Unexpected implicit 'this'!\n";
    // assert(false && "Unexpected implicit 'this'!");
  }

  // void VisitMemberExpr(MemberExpr *E) {
    // if(CXXThisExpr *Te = dyn_cast_or_null<CXXThisExpr>(E->getBase())) {
    //   if(Te->isImplicit()) {
    //       noteZ1Ref(E);
    //       return;
    //   }
    // }
  //   Base::VisitMemberExpr(E);
  // }

  void VisitDeclRefExpr(DeclRefExpr *E) {
    if(isDezombiefyCandidate(E))
      noteZ1Ref(E);
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

  // void VisitBinComma(clang::BinaryOperator *E) {
  //   // C++11 [expr.comma]p1:
  //   //   Every value computation and side effect associated with the left
  //   //   expression is sequenced before every value computation and side
  //   //   effect associated with the right expression.

  //   auto Raii = beginSequenced();
  //   Base::VisitBinComma(E);
  // }

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

  // void VisitBinAssign(clang::BinaryOperator *E) {
  //     //C++17
  //   // In every simple assignment expression E1=E2 and
  //   // every compound assignment expression E1@=E2,
  //   // every value computation and side-effect of E2
  //   // is sequenced before every value computation and side effect of E1
  //   RegionRaii RR(Tree, Region);

  //   RR.beginNewRegion();
  //   Visit(E->getLHS());

  //   RR.beginNewRegion();
  //   Visit(E->getRHS());
  // }

  // void VisitCompoundAssignOperator(CompoundAssignOperator *CAO) {
  //   VisitBinAssign(CAO);
  // }

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
  // void VisitBinLOr(BinaryOperator *E) {
  //   // The side-effects of the LHS of an '&&' are sequenced before the
  //   // value computation of the RHS, and hence before the value computation
  //   // of the '&&' itself, unless the LHS evaluates to zero. We treat them
  //   // as if they were unconditionally sequenced.
  //   RegionRaii RR(Tree, Region);

  //   RR.beginNewRegion();
  //   Visit(E->getLHS());


  //   // if it will short-circuit, don't go into RHS
  //   auto R = evaluate(E->getLHS());
  //   if(R == True)
  //       return;

  //   RR.beginNewRegion();
  //   Visit(E->getRHS());
  // }
  // void VisitBinLAnd(BinaryOperator *E) {
          
          
  //   RegionRaii RR(Tree, Region);
    
  //   RR.beginNewRegion();
  //   Visit(E->getLHS());

  //   // if it will short-circuit, don't go into RHS
  //    auto R = evaluate(E->getLHS());
  //     if (R == False)
  //       return;

  //   RR.beginNewRegion();
  //   Visit(E->getRHS());
  // }

  // void VisitAbstractConditionalOperator(AbstractConditionalOperator *E) {

  //   RegionRaii RR(Tree, Region);

  //   RR.beginNewRegion();
  //   Visit(E->getCond());

  //   // if we know only one branck will be evaluated, don't visit the other
  //    auto R = evaluate(E->getCond());

  //   if(R == True || R == Unknown) {
  //       RR.beginNewRegion();
  //       Visit(E->getTrueExpr());
  //   }

  //   if(R == False || R == Unknown) {
  //       RR.beginNewRegion();
  //       Visit(E->getFalseExpr());

  //   }
  // }

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
            Qt.dump();
            auto Rd = Qt->getAsCXXRecordDecl();
            bool SysType = isInSystemHeader(Context, Rd);
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
    noteCall(E);

    bool AtLeastOneArgumentMayZombie = false;
    bool BaseMayZombie = false;

    if(auto D = E->getDirectCallee()) {
      bool IsSystem = isInSystemHeader(Context, D);

      auto Ce = E->getCallee()->IgnoreParenCasts();
      
      //mb: here we need to analyze what is going to be the 
      // 'this' in the called method, to see if we can 
      // verify it will not be a zombie
      if(!Ce) {
        // not really sure this can happend
        assert(false);
      }
      else if(auto Me = dyn_cast<MemberExpr>(Ce) ) {
        
        BaseMayZombie = mayBaseZombie(Me) ;
        
      }
      else if(isa<DeclRefExpr>(Ce)) {
        //this is a function call, no need to check anything
      }
      else {
        Ce->dumpColor();
        assert(false);
      }

      Visit(E->getCallee());

      vector<bool> P;
      unsigned Count = D->getNumParams();
      //TODO on CXXOperatorCallExpr, the callee expr
      // is an argument. then the count mismatch
      unsigned Offset = E->getNumArgs() == Count + 1 ? 1 : 0;
      assert(E->getNumArgs() == Count + Offset);

      for(unsigned I = 0; I != Count; ++I) {
        auto Each = D->getParamDecl(I);
        auto EachE = E->getArg(I + Offset);

        // for argument to zombie, the type called declaration must be a ref or ptr
        // and it must be initialized from something not being a plain value
        bool Z = mayZombie(Each->getType());
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
        //check for issue Z3 
        for(unsigned I = 0; I != Count; ++I) {
          auto Each = D->getParamDecl(I);
          auto Qt = Each->getType();
          if (isByValueUserTypeNonTriviallyCopyable(Context, Qt)) {
              //report a Z3 issue here.

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
    else {
      // not sure how why this can happend
      E->dumpColor();
      assert(false);
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


ZombieSequence checkSequence(clang::ASTContext &Context, clang::Expr *E, ZombieSequence ZqMax, bool ReportAll) {

  SequenceCheckASTVisitor2 V(Context, ZqMax, ReportAll);
  V.Visit(E);
  return V.getIssue();
}

} // namespace nodecpp

#endif // NODECPP_CHECKER_SEQUENCECHECKASTVISITOR_H

