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

#ifndef NODECPP_CHECKER_DEUNSEQUENCEASTVISITOR_H
#define NODECPP_CHECKER_DEUNSEQUENCEASTVISITOR_H

#include "SequenceCheck.h"

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
using namespace llvm;
using namespace std;


template<class T>
class RecursivePostOrderASTVisitor : public RecursiveASTVisitor<T> {
  public:
    /// Return whether this visitor should traverse post-order.
  bool shouldTraversePostOrder() const { return true; }
};

class ExpressionUnwrapperVisitor : public RecursivePostOrderASTVisitor<ExpressionUnwrapperVisitor>{

  const ASTContext *Context = nullptr;
  string Buffer;
  int Index = 0;
  vector<pair<string, SourceRange>> Reps;
public:

  ExpressionUnwrapperVisitor(const ASTContext *Context) :Context(Context) {}

  // mb: copy and paste from lib/AST/StmtPrinter.cpp
  static pair<bool, StringRef> printExprAsWritten(Expr *E,
                                const ASTContext *Context) {
    if (!Context)
      return {false, ""};
    bool Invalid = false;
    StringRef Source = Lexer::getSourceText(
        CharSourceRange::getTokenRange(E->getSourceRange()),
        Context->getSourceManager(), Context->getLangOpts(), &Invalid);
    return {!Invalid, Source};
  }

  void unwrap(Expr *E) {

    vector<pair<string, SourceRange>> Tmp;
    swap(Reps, Tmp);

    auto R = printExprAsWritten(E, Context);
    if(!R.first)
      return;

    string Name = generateName();
    Buffer += "auto ";
    Buffer += Name;
    Buffer += " = ";
    //here do magic to replace existing replacemnts
    Buffer += R.second;
    Buffer += ";";

    addReplacement(Name, E->getSourceRange());
  }

  void addReplacement(string text, SourceRange Sr) {
    Reps.emplace_back(text, Sr);
  }

  bool hasReplacement() const {
    return !Reps.empty();
  }

  bool hasInsertion() const {
    return !Buffer.empty();
  }

  tooling::Replacement makeInsert(SourceLocation Where) {

    auto Sr = CharSourceRange::getCharRange(Where, Where);
    tooling::Replacement R(Context->getSourceManager(), Sr,
                                      Buffer);
    return R;
  } 

  tooling::Replacement makeFix() {
    auto Sr = CharSourceRange::getTokenRange(Reps.back().second);
    tooling::Replacement R(Context->getSourceManager(), Sr,
                                      Reps.back().first);
                                   
    Reps.pop_back();
    assert(Reps.empty());
    return R;
  }

  string generateName() {
    string Name = "nodecpp_" + to_string(Index);
    ++Index;
    return Name;
  }

  // bool VisitDeclRefExpr(DeclRefExpr *Dre) {
    
  // }

  // bool VisitCXXThisExpr(CXXThisExpr *E) {
  // }

  bool VisitCallExpr(CallExpr *E) {
    unwrap(E);
    return RecursiveASTVisitor<ExpressionUnwrapperVisitor>::VisitCallExpr(E);
  }

  bool VisitBinaryOperator(BinaryOperator *E) {
    unwrap(E);
    return RecursiveASTVisitor<ExpressionUnwrapperVisitor>::VisitBinaryOperator(E);
  }

  bool VisitUnaryOperator(UnaryOperator *E) {
    unwrap(E);
    return RecursiveASTVisitor<ExpressionUnwrapperVisitor>::VisitUnaryOperator(E);
  }

};




class Deunsequence2ASTVisitor
  : public clang::RecursiveASTVisitor<Deunsequence2ASTVisitor> {

    using Base = clang::RecursiveASTVisitor<Deunsequence2ASTVisitor>;
  clang::ASTContext &Context;
//  DzHelper &DzData;

  /// Fixes to apply, grouped by file path.
  llvm::StringMap<clang::tooling::Replacements> FileReplacements;

  void addFix(const tooling::Replacement& Replacement) {
    llvm::Error Err = FileReplacements[Replacement.getFilePath()].add(Replacement);
    // FIXME: better error handling (at least, don't let other replacements be
    // applied).
    if (Err) {
      llvm::errs() << "Fix conflicts with existing fix! "
                    << llvm::toString(std::move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }

  void unwrapExpression(Stmt* St, Expr* E) {

    ExpressionUnwrapperVisitor V(&Context);
    V.TraverseStmt(E);
    if(V.hasReplacement()) {
      addFix(V.makeFix());
    }
    if(V.hasInsertion()) {
      addFix(V.makeInsert(St->getBeginLoc()));
    }
  }

public:
  const auto& getReplacements() const { return FileReplacements; }

  explicit Deunsequence2ASTVisitor(clang::ASTContext &Context):
    Context(Context) {}


  // bool TraverseStmt(Stmt *St) {

  //   if(Expr* E = dyn_cast_or_null<Expr>(St)) {
  //     unwrapExpression(St, E);
  //     return true;
  //   }
  //   else
  //     return clang::RecursiveASTVisitor<Deunsequence2ASTVisitor>::TraverseStmt(St);
  // }

  // bool TraverseDeclStmt(DeclStmt *St) {
    
  //   if(St->isSingleDecl()) {
  //     if(VarDecl* D = dyn_cast_or_null<VarDecl>(St->getSingleDecl())) {
  //       if(Expr *E = D->getInit()) {
  //         unwrapExpression(St, E);
  //       }
  //     }
  //   }
  //   return RecursiveASTVisitor<Deunsequence2ASTVisitor>::TraverseDeclStmt(St);
  // }

  // bool TraverseWhileStmt(WhileStmt *St) {
  //   unwrapExpression(St, St->getCond());
  //   return RecursiveASTVisitor<Deunsequence2ASTVisitor>::TraverseWhileStmt(St);
  // }

  bool TraverseStmt(Stmt *St) {
    // For every root expr, sent it to check and don't traverse it here
    if(!St)
      return true;

    if(Expr *E = dyn_cast<Expr>(St)) {
//      checkUnsequencedDezombiefy(Context, E, true);
      return true;
    }
    else
      return Base::TraverseStmt(St);
  }

  // bool VisitStmt(Stmt *St) {

  //   // if(Expr* E = dyn_cast_or_null<Expr>(St)) {
  //   //   unwrapExpression(E, E);
  //   //   return true;
  //   // }
  //   // else
  //     return clang::RecursiveASTVisitor<Deunsequence2ASTVisitor>::VisitStmt(St);
  // }
};

} // namespace nodecpp

#endif // NODECPP_CHECKER_DEUNSEQUENCEASTVISITOR_H

