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


enum class ZombieSequence {NONE = 0, Z1, Z2, Z9};

class ZombieIssues {
  std::vector<std::pair<Expr *, Expr *>> Z1Issues;
  std::vector<std::pair<Expr *, Expr *>> Z2Issues;
  std::vector<std::pair<Expr *, Expr *>> Z9Issues;

public:
  unsigned getMessageId(ZombieSequence Zs) {
    
  }

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
  void reportIssue(DiagnosticsEngine &De, ZombieSequence Zs,
    const Expr *E1, const Expr *E2) {

    unsigned ID =De.getDiagnosticIDs()->getCustomDiagID(
      DiagnosticIDs::Error, 
      "(%0) Dezombiefication not fully realiable [nodecpp-dezombiefy]");

    if(Zs == ZombieSequence::Z1) {
      De.Report(E1->getExprLoc(), ID) << "Z1" << E2->getExprLoc();
    }
    else if(Zs == ZombieSequence::Z2) {
      De.Report(E1->getExprLoc(), ID) << "Z2" << E2->getExprLoc();
    }
    else if(Zs == ZombieSequence::Z9) {
      De.Report(E1->getExprLoc(), ID) << "Z9" << E2->getExprLoc();

      unsigned IDNote = De.getDiagnosticIDs()->getCustomDiagID(
        DiagnosticIDs::Note, 
        "Non-trivial copy/move constructor of type %0");
      De.Report(E1->getExprLoc(),IDNote) << E1->getType();
    }
    // E1->dump();
    // E2->dumpColor();
  }

  void reportIssue(DiagnosticsEngine &De) const {
    if(!Z9Issues.empty()) {
      reportIssue(De, ZombieSequence::Z9,
        Z9Issues.front().first, Z9Issues.front().second);
    }
    else if(!Z2Issues.empty()) {
      reportIssue(De, ZombieSequence::Z2,
        Z2Issues.front().first, Z2Issues.front().second);
    }
    else if(!Z1Issues.empty()) {
      reportIssue(De, ZombieSequence::Z1,
        Z1Issues.front().first, Z1Issues.front().second);
    }
  }

  void reportAllIssues(DiagnosticsEngine &De) const {
    
    for(auto &Each : Z9Issues)
      reportIssue(De, ZombieSequence::Z9, Each.first, Each.second);
    for(auto &Each : Z2Issues)
      reportIssue(De, ZombieSequence::Z2, Each.first, Each.second);
    for(auto &Each : Z1Issues)
      reportIssue(De, ZombieSequence::Z1, Each.first, Each.second);
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
  bool SilentMode = false;

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
  SequenceCheckASTVisitor2(ASTContext &Context, bool SilentMode)
      : Base(Context), Context(Context), SilentMode(SilentMode), Region(Tree.root()) {
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

  template<class EXPR, class DECL>
  void CallHelper(EXPR* E, DECL* D, bool BaseMayZombie) {

    D = D->getCanonicalDecl();
    SmallVector<QualType, 4> ArgsT;
    bool IsSystem = needsExternalDezombiefy(Context, D);
    for(auto &Each : D->parameters()) {
      ArgsT.push_back(Each->getType());
    }

    bool AtLeastOneArgumentMayZombie = false;

    vector<bool> P;
    unsigned Count = ArgsT.size();
    //TODO on CXXOperatorCallExpr, the callee expr
    // is an argument. then the count mismatch
    unsigned Offset = E->getNumArgs() == Count + 1 ? 1 : 0;
    assert(E->getNumArgs() == Count + Offset);


    for(unsigned I = 0; I != Count; ++I) {
      auto EachT = ArgsT[I];
      auto EachE = E->getArg(I + Offset);

      // for argument to zombie, the type of called declaration must be a ref or ptr
      // and it must be initialized from something not being a plain value or literal
      bool Z = mayZombie(EachT);
      if(Z) {
        EachE = EachE->IgnoreParenCasts();
        if(isLiteralExpr(EachE)) {
          Z = false;
        }
        else if(auto Dre = dyn_cast<DeclRefExpr>(EachE)) {
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
        auto EachT = ArgsT[I];
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
    
    auto Raii = beginSequenced();

    bool BaseMayZombie = false;

    Expr *Ce = E->getCallee()->IgnoreParenCasts();
    
    //mb: here we need to analyze what is going to be the 
    // 'this' in the called method, to see if we can 
    // verify it will not be a zombie
    if(!Ce) {
      // not really sure this can happend
      llvm::errs() << "!E->getCallee()\n";
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
    else if(auto Bo = dyn_cast<BinaryOperator>(Ce)) {
      //TODO
      //this is a pointer to member or something extrange
      // right now just ignore
      BaseMayZombie = false;

    }
    else {
      llvm::errs() << "!unknown callee()\n";
      Ce->dumpColor();
      assert(false);
      return;
    }

    auto D = E->getDirectCallee();
    if(!D) {
      //TODO
      // this can happend with pointer to member kind of things
      // we can't really know where it is going to land
      // right now, just don't do anything else here

//      llvm::errs() << "!E->getDirectCallee()\n";
      return;
    }

    noteCall(E);
    Visit(E->getCallee());

    CallHelper(E, D, BaseMayZombie);
  }

  void VisitCXXConstructExpr(CXXConstructExpr *E) {

    // In C++11, list initializations are sequenced.
    // In C++17, other constructor as function call

    auto Raii = beginSequenced();

    CallHelper(E, E->getConstructor(), false);
  }

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


bool checkSequence(clang::ASTContext &Context, clang::Expr *E, ZombieSequence ZqMax, bool DebugReportMode, bool SilentMode, ZombieIssuesStats& Stats) {

  // for constant expressions, we assume they are ok
  if(E->isCXX11ConstantExpr(Context))
    return false;

  SequenceCheckASTVisitor2 V(Context, SilentMode);
  V.Visit(E);
  auto &Issues = V.getIssues();
  Issues.addStats(Stats);
  if(Issues.getMaxIssue() == ZombieSequence::NONE) {
    //no issues, no fixes
    return false;
  }
  else if(DebugReportMode) {
    //report all things the checker matched
    Issues.reportAllIssues(Context.getDiagnostics());
    return false;
  }
  else if(Issues.getMaxIssue() <= ZqMax) {
    //we can make this work
    if(ZqMax == ZombieSequence::Z1)
      Stats.Op2CallFixCount++;
    else if(ZqMax == ZombieSequence::Z2)
      Stats.UnwrapFixCount++;

    return true;
  }
  else {
    //can't make the fix we need
    if(!SilentMode)
      Issues.reportIssue(Context.getDiagnostics());

    if(Issues.getMaxIssue() == ZombieSequence::Z9)
      Stats.UnfixedZ9Count++;
    else if(Issues.getMaxIssue() == ZombieSequence::Z2)
      Stats.UnfixedZ2Count++;

    //but make the fix we can, is better than nothing
    return true;
  }
}

} // namespace nodecpp

#endif // NODECPP_CHECKER_SEQUENCECHECKASTVISITOR_H

