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

#ifndef NODECPP_CHECKER_SEQUENCEFIXASTVISITOR_H
#define NODECPP_CHECKER_SEQUENCEFIXASTVISITOR_H

#include "SequenceCheck.h"

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


class SequenceFixASTVisitor
  : public EvaluatedExprVisitor<SequenceFixASTVisitor> {

  using Base = EvaluatedExprVisitor<SequenceFixASTVisitor>;
  ASTContext &Context;
  /// Fixes to apply
  Replacements FileReplacements;
  SmallVector<Replacement, 6> MoreReplacements;

  void addReplacement(const Replacement& Replacement) {
    Error Err = FileReplacements.add(Replacement);
    if (Err) {
      errs() << "Fix conflicts with existing fix! "
                    << toString(move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }

  /// function names usually overlap, we keep them in order
  /// here, and merge them as needed
  void addReplacement2(const Replacement& R) {
    if(!MoreReplacements.empty()) {
      auto &Last = MoreReplacements.back();
      if(Last.getFilePath() == R.getFilePath() &&
        Last.getOffset() == R.getOffset() &&
        Last.getLength() == 0 && R.getLength() == 0) {

        //merge them
        SmallString<48> Buffer;
        Buffer += Last.getReplacementText();
        Buffer += R.getReplacementText();

        Replacement N(Last.getFilePath(), Last.getOffset(),
          Last.getLength(), Buffer);

        MoreReplacements.pop_back();
        MoreReplacements.push_back(N);
        return;
      }
    }
    MoreReplacements.push_back(R);
  }

public:
  auto& finishReplacements() { 
    
    for(auto& Each : MoreReplacements) {
      addReplacement(Each);
    }
    return FileReplacements;
  }

  explicit SequenceFixASTVisitor(clang::ASTContext &Context):
    Base(Context), Context(Context) {}


  void refactorBinaryOperator(BinaryOperator *E, StringRef Text) {

    
    SmallString<24> Ss;
    Ss += "nodecpp::safememory::";
    Ss += Text;
    Ss += "(";
    Replacement R0(Context.getSourceManager(), E->getBeginLoc(), 0, Ss);

    size_t Sz = E->getOpcodeStr().size();
    Replacement R1(Context.getSourceManager(), E->getOperatorLoc(), Sz, ",");

    Replacement R2(Context.getSourceManager(), E->getEndLoc(), 0, ")");
    // move one char to the left, to insert after EndLoc
    Replacement R3(R2.getFilePath(), R2.getOffset() + 1, 0, ")");

    /// the operator and the closing paren are never conflicting.
    addReplacement2(R0);
    addReplacement(R1);
    addReplacement(R3);
  }

  void VisitBinaryOperator(BinaryOperator *E) {
    switch (E->getOpcode()) {
      case BO_PtrMemD:
      case BO_PtrMemI:
        break;
      case BO_Mul:
        refactorBinaryOperator(E, "mul");
        break;
      case BO_Div:
        refactorBinaryOperator(E, "div");
        break;
      case BO_Rem:
        refactorBinaryOperator(E, "rem");
        break;
      case BO_Add:
        refactorBinaryOperator(E, "add");
        break;
      case BO_Sub:
        refactorBinaryOperator(E, "sub");
        break;
      case BO_Shl:
      case BO_Shr:
        break;
      case BO_LT:
        refactorBinaryOperator(E, "lt");
        break;
      case BO_GT:
        refactorBinaryOperator(E, "gt");
        break;
      case BO_LE:
        refactorBinaryOperator(E, "le");
        break;
      case BO_GE:
        refactorBinaryOperator(E, "ge");
        break;
      case BO_EQ:
        refactorBinaryOperator(E, "eq");
        break;
      case BO_NE:
        refactorBinaryOperator(E, "ne");
        break;

      case BO_Cmp:
        refactorBinaryOperator(E, "cmp");
        break;
      case BO_And:
        refactorBinaryOperator(E, "and");
        break;
      case BO_Xor:
        refactorBinaryOperator(E, "xor");
        break;
      case BO_Or :
        refactorBinaryOperator(E, "or");
        break;
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
        break;
    }

    Base::VisitBinaryOperator(E);
  }
};

} // namespace nodecpp

#endif // NODECPP_CHECKER_SEQUENCEFIXASTVISITOR_H

