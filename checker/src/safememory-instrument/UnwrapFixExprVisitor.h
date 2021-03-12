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

#ifndef NODECPP_CHECKER_UNWRAPFIXEXPRVISITOR_H
#define NODECPP_CHECKER_UNWRAPFIXEXPRVISITOR_H

#include "CodeChange.h"
#include "BaseASTVisitor.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
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
//using namespace clang::tooling;
using namespace llvm;
using namespace std;


// template<class T>
// class RecursivePostOrderASTVisitor : public RecursiveASTVisitor<T> {
//   public:
//     /// Return whether this visitor should traverse post-order.
//   bool shouldTraversePostOrder() const { return true; }
// };

class UnwrapFixExprVisitor : public clang::EvaluatedExprVisitor<UnwrapFixExprVisitor> {

  using Base = clang::EvaluatedExprVisitor<UnwrapFixExprVisitor>;

  FileChanges &FileReplacements;

  bool SilentMode = false;
  bool HasError = false;
  int &Index;

  string StmtText;
  Range StmtRange;
  CharSourceRange StmtSourceRange;
  SourceLocation StmtCloseBrace;
  string Buffer; //TODO change to ostream or similar
  bool ExtraSemicolon = false;
  bool ExtraBraces = false;
  
  list<pair<string, Range>> Reps;

  void addReplacement(const CodeChange& Replacement) {
    auto Err = FileReplacements.add(Context.getSourceManager(), Replacement);
    if(Err) {
      HasError = true;
      auto Str = llvm::toString(std::move(Err));
      if(!SilentMode) {
        llvm::errs() << "Fix conflicts with existing fix!\n" << Str << "\n";
      }
    }
  }

  // mb: copy and paste from lib/AST/StmtPrinter.cpp
  static pair<bool, StringRef> printExprAsWritten(CharSourceRange ChRange,
                                const ASTContext &Context) {
    // if (!Context)
    //   return {false, ""};
    bool Invalid = false;
    StringRef Source = Lexer::getSourceText(ChRange,
        Context.getSourceManager(), Context.getLangOpts(), &Invalid);
    return {!Invalid, Source};
  }

  bool isContained(unsigned Offset, unsigned Length, const Range& RHS) {
    return RHS.getOffset() >= Offset &&
           (RHS.getOffset() + RHS.getLength()) <= (Offset + Length);
  }

  string subStmtWithReplaces(Range RangeInStmtText) {

    //here do magic to replace existing replacemnts
    string B = StmtText;
    unsigned Offset = RangeInStmtText.getOffset();
    unsigned Length =  RangeInStmtText.getLength();



    for(auto It = Reps.crbegin(); It != Reps.crend(); ++It) {

      if(isContained(Offset, Length, It->second)) {
        Length += It->first.size();
        Length -= It->second.getLength();
        B.replace(It->second.getOffset(), It->second.getLength(), It->first);
      }
    }

    return B.substr(Offset, Length);
  }

  void unwrap(Expr *E, bool AddStar = false) {

    if(StmtText.empty())
      return;

    E = E->IgnoreParenImpCasts();
    bool LValue = E->isLValue();
    bool TrivialType = E->getType().getCanonicalType().isTrivialType(Context);

    if(isa<DeclRefExpr>(E))
      return;
    else if(isa<CXXThisExpr>(E))
      return;
    else if(isLiteralExpr(E))
      return;
    else if(isa<CXXDefaultArgExpr>(E))
      return;
    else {
      auto C = dyn_cast<CallExpr>(E);
      if(C && C->isCallToStdMove())
        return;

      auto Ctr = dyn_cast<CXXConstructExpr>(E);
      if(Ctr)
        return;

    }

    auto Sr = toCheckedCharRange(E->getSourceRange(),
      Context.getSourceManager(), Context.getLangOpts());

    if(!Sr.isValid()) {
      reportError(E->getExprLoc());
      HasError = true;
      return;
    }

    Range R = calcRange(Sr);

    Range RangeInStmtText = toTextRange(R);

    if(RangeInStmtText == Range() || 
      RangeInStmtText.getOffset() + RangeInStmtText.getLength() > StmtText.size()) {
      //mb: this shouldn't happend

//      assert(false);
      llvm::errs() << "unwrap() error at 'toTextRange(R)'\n";
      return;

    }

    string Name = generateName();

    if(LValue) 
      Buffer += "auto& ";
    else if(TrivialType)
      Buffer += "auto ";
    else
      Buffer += "auto&& ";
    
    Buffer += Name;
    Buffer += " = ";
    if(AddStar)
      Buffer += "&*(";

    Buffer += subStmtWithReplaces(RangeInStmtText);
    if(AddStar)
      Buffer += ")";

    Buffer += "; ";

    addTextReplacement(move(Name), RangeInStmtText);
  }

  void unwrapStar(Expr *E) {
    unwrap(E, true);
  }

  Range calcRange(const CharSourceRange &Sr) {

    assert(Sr.isCharRange());

    auto& Sm = Context.getSourceManager();

    std::pair<FileID, unsigned> Start = Sm.getDecomposedLoc(Sr.getBegin());
    std::pair<FileID, unsigned> End = Sm.getDecomposedLoc(Sr.getEnd());
    
    assert (Start.first == End.first);
    assert (End.second >= Start.second);

    return Range(Start.second, End.second - Start.second);
  }

  Range toTextRange(Range SrcRange) {

    if(SrcRange.getOffset() < StmtRange.getOffset())
      return Range();

    return Range(SrcRange.getOffset() - StmtRange.getOffset(), SrcRange.getLength());

  }

  void addTextReplacement(string Text, Range R) {

    auto It = Reps.begin();
    while(It != Reps.end()) {
      if(R.overlapsWith(It->second)) {
        assert(R.contains(It->second));
        It = Reps.erase(It);
      }
      else
        ++It;
    }

    auto Jt = Reps.begin();
    while(Jt != Reps.end()) {
      if(R.getOffset() < Jt->second.getOffset()) {
        Reps.insert(Jt, {Text, R});
        return;
      }
      else
        ++Jt;
    }
    Reps.push_back({Text, R});
  }

  void makeFix() {

    Buffer += subStmtWithReplaces(Range(0, StmtText.size()));
    if(ExtraBraces) {
      Buffer.insert(0, "{ ");
      if(ExtraSemicolon)
        Buffer += ";";
    }

    addReplacement(CodeChange::makeReplace(
      Context.getSourceManager(), StmtSourceRange, Buffer));
    

    // we make two changes for the case of unwrapping if expr
    // we don't want the change to go over all of the if body
    if(ExtraBraces) {
      addReplacement(CodeChange::makeInsertRight(
        Context.getSourceManager(), StmtCloseBrace, " }"));

    }

  }

  string generateName() {
    string Name = "nodecpp_" + to_string(Index);
    ++Index;
    return Name;
  }

  void reportError(SourceLocation OpLoc) {

    if(SilentMode)
      return;

    DiagnosticsEngine &De = Context.getDiagnostics();

    unsigned ID =De.getDiagnosticIDs()->getCustomDiagID(
      DiagnosticIDs::Error, 
      "Unwrap couldn't complete because of MACRO [safememory-dezombiefy]");

      De.Report(OpLoc, ID);
  }

public:

  UnwrapFixExprVisitor(const clang::ASTContext &Context, bool SilentMode, 
    FileChanges &Replacements, int &Index)
    :Base(Context), FileReplacements(Replacements), SilentMode(SilentMode), Index(Index) {}

  bool unwrapExpression(const Stmt *St, Expr *E, bool ExtraB, bool ExtraS) {
    
    ExtraBraces = ExtraB;
    ExtraSemicolon = ExtraS;
    StmtCloseBrace = St->getEndLoc();

    StmtSourceRange = toCheckedCharRange({St->getBeginLoc(), E->getEndLoc()},
      Context.getSourceManager(), Context.getLangOpts());

    if(!StmtSourceRange.isValid()) {

      reportError(E->getExprLoc());
      return false;
    }

    StmtRange = calcRange(StmtSourceRange);

    auto R = printExprAsWritten(StmtSourceRange, Context);
    if(!R.first) {
      //mb: this shouldn't happend

//      assert(false);
      llvm::errs() << "unwrapExpression() error at 'printExprAsWritten()'\n";
      return false;
    }

    StmtText = R.second.str();

    if(StmtText.size() != StmtRange.getLength()) {
      //mb: this shouldn't happend

//      assert(false);
      llvm::errs() << "unwrapExpression() error at 'StmtText.size() != StmtRange.getLength()'\n";
      return false;
    }


    Visit(E);
    if(!Buffer.empty()) {
      makeFix();
    }

    return !HasError;
  }

  //special treatmeant for smart ptrs
  bool isOverloadArrowOp(Expr *E) {

    E = E->IgnoreParenImpCasts();
    auto Op = dyn_cast_or_null<CXXOperatorCallExpr>(E);
    return Op && Op->getOperator() == OverloadedOperatorKind::OO_Arrow;
  }

  void VisitMemberExpr(MemberExpr *E) {

    //special treatmeant for smart ptrs
    if(isOverloadArrowOp(E->getBase())) {
      unwrapStar(E->getBase());
    }
    else {
      Base::VisitMemberExpr(E);
    }
  }

  void VisitCallExpr(CallExpr *E) {

    Base::VisitCallExpr(E);

    auto Callee = dyn_cast_or_null<MemberExpr>(E->getCallee());
    if(Callee) {
      //special treatmeant for smart ptrs is at VisitMemberExpr
      if(!isOverloadArrowOp(Callee->getBase()))
        unwrap(Callee->getBase());
    }
  
    for(auto Each : E->arguments()) {
      // on CxxOperator callee may be also first arg
      if(Each != Callee) {
        unwrap(Each);
      }
      else {
        //mb: this shouldn't happend

//      assert(false);
        llvm::errs() << "unwrapExpression() error at 'if(Each != Callee)'\n";
        Each->dumpColor();
      }
    }
  }

  void VisitBinaryOperator(BinaryOperator *E) {

    Base::VisitBinaryOperator(E);

    if(E->isAdditiveOp() || E->isMultiplicativeOp() ||
      E->isBitwiseOp() || E->isComparisonOp() ||
      E->isRelationalOp()) {
        unwrap(E->getLHS());
        unwrap(E->getRHS());
      }
  }

  void VisitUnaryOperator(UnaryOperator *E) {

    Base::VisitUnaryOperator(E);

    if(E->isPostfix()) {
      unwrap(E->getSubExpr());
    }
  }

  void VisitCXXConstructExpr(CXXConstructExpr *E) {

    Base::VisitCXXConstructExpr(E);
  
    for(auto Each : E->arguments()) {
      unwrap(Each);
    }
  }

  void VisitCoawaitExpr(CoawaitExpr *E) {

    //TODO think what to do here
  }

};

bool needExtraBraces(clang::ASTContext &Context, const Stmt *St) {

  auto SList = Context.getParents(*St);

  auto SIt = SList.begin();

  if (SIt == SList.end())
    return true;

  return SIt->get<CompoundStmt>() == nullptr;
}



// This one is for stand-alone expression statements
bool applyUnwrapFix(const clang::ASTContext &Context, bool SilentMode, 
        FileChanges &Replacements, int &Index, clang::Expr* E) {

  UnwrapFixExprVisitor V2(Context, SilentMode, Replacements, Index);
  return V2.unwrapExpression(E, E, true, true);
}

// This one is for expression inside statements (i.e. condition on 'if' stmt)
bool applyUnwrapFix(clang::ASTContext &Context, bool SilentMode, 
        FileChanges &Replacements, int &Index, const clang::Stmt* St, clang::Expr* E) {

  bool ExtraBraces = needExtraBraces(Context, St);
  UnwrapFixExprVisitor V2(Context, SilentMode, Replacements, Index);
  return V2.unwrapExpression(St, E, ExtraBraces, false);
}

} // namespace nodecpp

#endif // NODECPP_CHECKER_UNWRAPFIXEXPRVISITOR_H

