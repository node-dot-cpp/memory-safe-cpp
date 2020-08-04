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

#ifndef NODECPP_CHECKER_OP2CALLFIXEXPRVISITOR_H
#define NODECPP_CHECKER_OP2CALLFIXEXPRVISITOR_H

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


class Op2CallFixExprVisitor
  : public clang::EvaluatedExprVisitor<Op2CallFixExprVisitor> {

  using Base = clang::EvaluatedExprVisitor<Op2CallFixExprVisitor>;

  /// Fixes to apply.
  FileChanges &FileReplacements;
  bool SilentMode = false;
  bool HasError = false;

  void addReplacement(const CodeChange& Replacement) {
    auto Err = FileReplacements.add(Context.getSourceManager(), Replacement);
    if (Err) {
      // we must handle it, even if not actually written :(
      HasError = true;
      auto Str = llvm::toString(std::move(Err));
      if(!SilentMode) {
        llvm::errs() << "Fix conflicts with existing fix!\n" << Str;
      }
    }
  }

  void reportError(SourceLocation OpLoc) {

    if(SilentMode)
      return;

    DiagnosticsEngine &De = Context.getDiagnostics();

    unsigned ID =De.getDiagnosticIDs()->getCustomDiagID(
      DiagnosticIDs::Error, 
      "Op2Call couldn't complete because of MACRO [nodecpp-dezombiefy]");

      De.Report(OpLoc, ID);
  }


  void refactorOperator(SourceRange Sr, SourceLocation OpLoc,
    unsigned OpSize, StringRef Text) {

    auto ChRange = toCheckedCharRange({OpLoc, OpLoc.getLocWithOffset(OpSize)},
    Context.getSourceManager(), Context.getLangOpts());

    if(!ChRange.isValid() || !OpLoc.isValid() || OpLoc.isMacroID()) {
      HasError = true;
      reportError(OpLoc);
      return;
    }


    SmallString<24> Ss;
    Ss += "safe_memory::dz_";
    Ss += Text;
    Ss += "(";
    auto R0 = CodeChange::makeInsertLeft(
      Context.getSourceManager(), Sr.getBegin(), Ss);

    auto R1 = CodeChange::makeReplace(
      Context.getSourceManager(), ChRange, ",");

    // Replacement R2(Context.getSourceManager(), Sr.getEnd(), 0, ")");
    // // move one char to the left, to insert after EndLoc
    // Replacement R3(R2.getFilePath(), R2.getOffset() + 1, 0, );
    auto R3 = CodeChange::makeInsertRight(
      Context.getSourceManager(), Sr.getEnd(), ")");

    /// the operator and the closing paren are never conflicting.
    addReplacement(R0);
    addReplacement(R1);
    addReplacement(R3);
  }

  void refactorBinaryOperator(BinaryOperator *E, unsigned OpSize, StringRef Text) {
    refactorOperator(E->getSourceRange(), E->getOperatorLoc(), OpSize, Text);
  }

  void refactorOverloadedOperator(CXXOperatorCallExpr *E, unsigned OpSize, StringRef Text) {
    refactorOperator(E->getSourceRange(), E->getOperatorLoc(), OpSize, Text);
  }

public:

  explicit Op2CallFixExprVisitor(const clang::ASTContext &Context, bool SilentMode, FileChanges &Replacements):
    Base(Context), FileReplacements(Replacements), SilentMode(SilentMode) {}

  bool fixExpression(clang::Stmt* St) {
    Visit(St);
    return !HasError;
  }

  void VisitBinaryOperator(BinaryOperator *E) {

    switch (E->getOpcode()) {
      case BO_Mul:
        refactorBinaryOperator(E, 1, "mul");
        break;
      case BO_Div:
        refactorBinaryOperator(E, 1, "div");
        break;
      case BO_Rem:
        refactorBinaryOperator(E, 1, "rem");
        break;
      case BO_Add:
        refactorBinaryOperator(E, 1, "add");
        break;
      case BO_Sub:
        refactorBinaryOperator(E, 1, "sub");
        break;
      case BO_LT:
        refactorBinaryOperator(E, 1, "lt");
        break;
      case BO_GT:
        refactorBinaryOperator(E, 1, "gt");
        break;
      case BO_LE:
        refactorBinaryOperator(E, 2, "le");
        break;
      case BO_GE:
        refactorBinaryOperator(E, 2, "ge");
        break;
      case BO_EQ:
        refactorBinaryOperator(E, 2, "eq");
        break;
      case BO_NE:
        refactorBinaryOperator(E, 2, "ne");
        break;
      case BO_Cmp:
        refactorBinaryOperator(E, 3, "cmp");
        break;
      case BO_And:
        refactorBinaryOperator(E, 1, "and");
        break;
      case BO_Xor:
        refactorBinaryOperator(E, 1, "xor");
        break;
      case BO_Or :
        refactorBinaryOperator(E, 1, "or");
        break;
      default:
        break;
    }

    Base::VisitBinaryOperator(E);
  }

  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *E) {
    //mb: for overloaded operators, we try to do the same thing
    // as for builtin operators, so templates that work on builtin
    // types and custom types can treat all the same
    
    if(E->getNumArgs() == 2) {

      switch (E->getOperator()) {
        case OO_Plus:
          refactorOverloadedOperator(E, 1, "add");
          break;
        case OO_Minus:
          refactorOverloadedOperator(E, 1, "sub");
          break;
        case OO_Star:
          refactorOverloadedOperator(E, 1, "mul");
          break;
        case OO_Slash:
          refactorOverloadedOperator(E, 1, "div");
          break;
        case OO_Percent:
          refactorOverloadedOperator(E, 1, "rem");
          break;
        case OO_Caret:
          refactorOverloadedOperator(E, 1, "xor");
          break;
        case OO_Amp:
          refactorOverloadedOperator(E, 1, "and");
          break;
        case OO_Pipe:
          refactorOverloadedOperator(E, 1, "or");
          break;
        case OO_Less:
          refactorOverloadedOperator(E, 1, "lt");
          break;
        case OO_Greater:
          refactorOverloadedOperator(E, 1, "gt");
          break;
        case OO_EqualEqual:
          refactorOverloadedOperator(E, 2, "eq");
          break;
        case OO_ExclaimEqual:
          refactorOverloadedOperator(E, 2, "ne");
          break;
        case OO_LessEqual:
          refactorOverloadedOperator(E, 2, "le");
          break;
        case OO_GreaterEqual:
          refactorOverloadedOperator(E, 2, "ge");
          break;
        case OO_Spaceship:
          refactorOverloadedOperator(E, 3, "cmp");
          break;
        default:
          break;
      }
    }

    Base::VisitCXXOperatorCallExpr(E);
  }
};

bool applyOp2CallFix(const clang::ASTContext &Context, bool SilentMode, FileChanges &Replacements, clang::Expr* E) {

  Op2CallFixExprVisitor V2(Context, SilentMode, Replacements);
  return V2.fixExpression(E);
}

} // namespace nodecpp

#endif // NODECPP_CHECKER_OP2CALLFIXEXPRVISITOR_H

