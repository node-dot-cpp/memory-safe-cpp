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

#ifndef NODECPP_CHECKER_UNWRAPPERASTVISITOR_H
#define NODECPP_CHECKER_UNWRAPPERASTVISITOR_H

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

class ExpressionUnwrapperVisitor : public clang::EvaluatedExprVisitor<ExpressionUnwrapperVisitor> {

  using Base = clang::EvaluatedExprVisitor<ExpressionUnwrapperVisitor>;

  bool SilentMode = false;
  bool HasError = false;
  int &Index;

  FileChanges &FileReplacements;

  string StmtText;
  Range StmtRange;
  SourceRange StmtSourceRange;
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
  static pair<bool, StringRef> printExprAsWritten(SourceRange R,
                                const ASTContext &Context) {
    // if (!Context)
    //   return {false, ""};
    bool Invalid = false;
    StringRef Source = Lexer::getSourceText(
        CharSourceRange::getTokenRange(R),
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
    else {
      auto C = dyn_cast<CallExpr>(E);
      if(C && C->isCallToStdMove())
        return;

      auto Ctr = dyn_cast<CXXConstructExpr>(E);
      if(Ctr)
        return;

    }

    Range RangeInStmtText = toTextRange(calcRange(E->getSourceRange()));
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

  Range calcRange(const SourceRange &Sr) {

    auto& Sm = Context.getSourceManager();
    SourceLocation SpellingBegin = Sm.getSpellingLoc(Sr.getBegin());
    SourceLocation SpellingEnd = Sm.getSpellingLoc(Sr.getEnd());
    
    std::pair<FileID, unsigned> Start = Sm.getDecomposedLoc(SpellingBegin);
    std::pair<FileID, unsigned> End = Sm.getDecomposedLoc(SpellingEnd);
    
    if (Start.first != End.first) return Range();

    //SourceRange is always in token
    End.second += Lexer::MeasureTokenLength(SpellingEnd, Sm, Context.getLangOpts());

    // const FileEntry *Entry = Sm.getFileEntryForID(Start.first);
    // this->FilePath = Entry ? Entry->getName() : InvalidLocation;
    return Range(Start.second, End.second - Start.second);
  }

  Range toTextRange(Range SrcRange) {

    if(SrcRange.getOffset() - StmtRange.getOffset() + SrcRange.getLength() <= StmtText.size())
      return Range(SrcRange.getOffset() - StmtRange.getOffset(), SrcRange.getLength());
    else {
      //macros messing things
      if(!SilentMode) {
        llvm::errs() << "toTextRange() error\n StmtText:'" << StmtText << "'\nSrcRange: " <<
          SrcRange.getOffset() << ", " << SrcRange.getLength() << "\nStmtRange: " <<
          StmtRange.getOffset() << ", " << StmtRange.getOffset() << "\n";
      }
      
      return StmtRange;
    }

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

public:

  ExpressionUnwrapperVisitor(const clang::ASTContext &Context, bool SilentMode, 
    FileChanges &Replacements, int &Index)
    :Base(Context), SilentMode(SilentMode), FileReplacements(Replacements), Index(Index) {}

  bool unwrapExpression(const Stmt *St, Expr *E, bool ExtraBraces, bool ExtraSemicolon) {
    
    this->FileReplacements.clear();
    this->Buffer.clear();
    this->Reps.clear();

    this->StmtSourceRange = {St->getBeginLoc(), E->getEndLoc()};
    this->StmtCloseBrace = St->getEndLoc();
    auto R = printExprAsWritten(StmtSourceRange, Context);
    if(!R.first)
      return false;

    this->StmtText = R.second;
    // when unwrapping if stmt, we must not go into the body of the if
    this->StmtRange = calcRange(StmtSourceRange);
    if(StmtText.size() != StmtRange.getLength()) {
      if(!SilentMode) {
        llvm::errs() << "unwrapExpression() error\n StmtText: " << StmtText.size() << ":'" <<
        StmtText << "'\nStmtRange: " << StmtRange.getOffset() << ", " << StmtRange.getLength() << "\n";
      }
//      St->dumpColor();
      return false;
//      assert(false);
    }


    this->ExtraBraces = ExtraBraces;
    this->ExtraSemicolon = ExtraSemicolon;

    Visit(E);
    if(!Buffer.empty()) {
      makeFix();
    }
    return !HasError;
  }

  //special treatmeant for smart ptrs
  bool isOverloadArrowOp(Expr *E) {

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
      else
        Each->dumpColor();

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

  ExpressionUnwrapperVisitor V2(Context, SilentMode, Replacements, Index);
  return V2.unwrapExpression(E, E, true, true);
}

// This one is for expression inside statements (i.e. condition on 'if' stmt)
bool applyUnwrapFix(clang::ASTContext &Context, bool SilentMode, 
        FileChanges &Replacements, int &Index, const clang::Stmt* St, clang::Expr* E) {

  bool ExtraBraces = needExtraBraces(Context, St);
  ExpressionUnwrapperVisitor V2(Context, SilentMode, Replacements, Index);
  return V2.unwrapExpression(St, E, ExtraBraces, false);
}

} // namespace nodecpp

#endif // NODECPP_CHECKER_UNWRAPPERASTVISITOR_H

