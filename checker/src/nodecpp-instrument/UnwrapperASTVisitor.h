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

  FileChanges FileReplacements;

  string StmtText;
  Range StmtRange;
  SourceRange StmtSourceRange;
  string Buffer; //TODO change to ostream or similar
  int &Index;
  bool ExtraSemicolon = false;
  bool ExtraBraces = false;
  
  list<pair<string, Range>> Reps;

  void addReplacement(const CodeChange& Replacement) {
    auto Err = FileReplacements.add(Context.getSourceManager(), Replacement);
    if (Err) {
      llvm::errs() << "Fix conflicts with existing fix! "
                    << llvm::toString(std::move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }

  // mb: copy and paste from lib/AST/StmtPrinter.cpp
  static pair<bool, StringRef> printExprAsWritten(Stmt *St,
                                const ASTContext &Context) {
    // if (!Context)
    //   return {false, ""};
    bool Invalid = false;
    StringRef Source = Lexer::getSourceText(
        CharSourceRange::getTokenRange(St->getSourceRange()),
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

  void unwrap(Expr *E) {

    if(StmtText.empty())
      return;

    bool LValue = E->isLValue();
    E = E->IgnoreParenImpCasts();
    if(isa<DeclRefExpr>(E))
      return;
    else if(isa<IntegerLiteral>(E))
      return;
    else if(isa<clang::StringLiteral>(E))
      return;
    else if(isa<CXXNullPtrLiteralExpr>(E))
      return;
    else if(isa<FloatingLiteral>(E))
      return;

    Range RangeInStmtText = toTextRange(calcRange(E->getSourceRange()));
    string Name = generateName();

    if(LValue) 
      Buffer += "auto& ";
    else
      Buffer += "auto&& ";
    
    Buffer += Name;
    Buffer += " = ";
    Buffer += subStmtWithReplaces(RangeInStmtText);
    Buffer += "; ";

    addTextReplacement(move(Name), RangeInStmtText);
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

    assert(SrcRange.getOffset() - StmtRange.getOffset() + SrcRange.getLength() <= StmtText.size());
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

      Buffer += " }";
    }

    addReplacement(CodeChange::makeReplace(
      Context.getSourceManager(), StmtSourceRange, Buffer));
  }

  string generateName() {
    string Name = "nodecpp_" + to_string(Index);
    ++Index;
    return Name;
  }

public:

  ExpressionUnwrapperVisitor(const clang::ASTContext &Context, int &Index)
    :Base(Context), Index(Index) {}

  FileChanges& unwrapExpression(Stmt *St, Expr *E, bool ExtraBraces, bool ExtraSemicolon) {
    
    this->FileReplacements.clear();
    this->Buffer.clear();
    this->Reps.clear();

    auto R = printExprAsWritten(St, Context);
    if(!R.first)
      return FileReplacements;

    this->StmtText = R.second;
    this->StmtSourceRange = St->getSourceRange();
    this->StmtRange = calcRange(StmtSourceRange);
    this->ExtraBraces = ExtraBraces;
    this->ExtraSemicolon = ExtraSemicolon;

    Visit(E);
    if(!Buffer.empty()) {
      makeFix();
    }
    return FileReplacements;
  }


  void VisitCallExpr(CallExpr *E) {

//      unwrap(E->getCallee());
//    if(E->getNumArgs() > 1) {
      for(auto Each : E->arguments()) {
        unwrap(Each);
      }
//    }
  }

  void VisitBinaryOperator(BinaryOperator *E) {

    if(E->isAdditiveOp() || E->isMultiplicativeOp() ||
      E->isBitwiseOp() || E->isComparisonOp() ||
      E->isRelationalOp()) {
        unwrap(E->getLHS());
        unwrap(E->getRHS());
      }
  }

  void VisitUnaryOperator(UnaryOperator *E) {
    
    if(E->isPostfix()) {
      unwrap(E->getSubExpr());
    }
  }

};



} // namespace nodecpp

#endif // NODECPP_CHECKER_UNWRAPPERASTVISITOR_H

