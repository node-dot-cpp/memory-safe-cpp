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

#include "SequenceCheck.h"
#include "DezombiefyHelper.h"
#include "SequenceCheckASTVisitor.h"
#include "SequenceFixASTVisitor.h"

#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Sema/Sema.h"

namespace nodecpp {

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;


class SequenceCheck2ASTVisitor
  : public clang::RecursiveASTVisitor<SequenceCheck2ASTVisitor> {

  using Base = clang::RecursiveASTVisitor<SequenceCheck2ASTVisitor>;

  ASTContext &Context;
  bool FixAll;

  /// Fixes to apply.
  Replacements FileReplacements;

    void addReplacement(const Replacement& Replacement) {
    Error Err = FileReplacements.add(Replacement);
    if (Err) {
      errs() << "Fix conflicts with existing fix! "
                    << toString(move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }


public:
  explicit SequenceCheck2ASTVisitor(ASTContext &Context, bool FixAll):
    Context(Context), FixAll(FixAll) {}

  const auto& getReplacements() const { return FileReplacements; }

  bool TraverseDecl(Decl* D) {
    //mb: we don't traverse decls in system-headers
    if(isInSystemHeader(Context, D))
      return true;
    else
      return Base::TraverseDecl(D);
  }

  bool TraverseStmt(Stmt *St) {
    // For every root expr, sent it to check and don't traverse it here
    if(!St)
      return true;
    else if(Expr *E = dyn_cast<Expr>(St)) {
//      E->dumpColor();
      SequenceCheckASTVisitor V(Context, false);
      V.Visit(E);
      if(FixAll || V.foundIssues()) {
        SequenceFixASTVisitor V2(Context);
        V2.Visit(E);
        for(auto& Each : V2.finishReplacements())
          addReplacement(Each);

//        overwriteChangedFiles(Context, V2.getReplacements());
     }

      return true;
    }
    else
      return Base::TraverseStmt(St);
  }
};


void overwriteChangedFiles(ASTContext &Context, const Replacements &Replaces, StringRef Name) {

  if(!Replaces.empty()) {
    Rewriter Rewrite(Context.getSourceManager(), Context.getLangOpts());

    if (!applyAllReplacements(Replaces, Rewrite)) {
      llvm::errs() << "Can't apply replacements for file " 
      << Replaces.begin()->getFilePath() << "\n";
    }
    if (Rewrite.overwriteChangedFiles()) {
      llvm::errs() << Name << " failed to apply suggested fixes to file " 
      << Replaces.begin()->getFilePath() << "\n";
    } else {
      llvm::errs() << Name << " applied suggested fixes.\n";
    }
  }
}


void overwriteChangedFiles(ASTContext &Context, const StringMap<Replacements> &FileReplacements, StringRef Name) {

  if(!FileReplacements.empty()) {
    for (const auto &FileAndReplacements : FileReplacements) {
      overwriteChangedFiles(Context, FileAndReplacements.second, Name);
    }
  }
}

void dezombiefySequenceCheckAndFix(ASTContext &Context, bool FixAll) {
      
  SequenceCheck2ASTVisitor Visitor1(Context, FixAll);

  Visitor1.TraverseDecl(Context.getTranslationUnitDecl());

  overwriteChangedFiles(Context, Visitor1.getReplacements(), "nodecpp-unsequenced");
}


} //namespace nodecpp