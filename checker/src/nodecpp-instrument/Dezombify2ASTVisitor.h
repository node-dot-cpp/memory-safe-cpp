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

#ifndef NODECPP_CHECKER_DEZOMBIFY2ASTVISITOR_H
#define NODECPP_CHECKER_DEZOMBIFY2ASTVISITOR_H

#include "DezombiefyRelaxASTVisitor.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Format/Format.h"


namespace nodecpp {

class Dezombify2ASTVisitor
  : public clang::RecursiveASTVisitor<Dezombify2ASTVisitor> {

  clang::ASTContext &Context;
  DzHelper &DzData;

  /// Fixes to apply, grouped by file path.
  llvm::StringMap<clang::tooling::Replacements> FileReplacements;


  void addFix(const clang::FixItHint& FixIt) {

    clang::CharSourceRange Range = FixIt.RemoveRange;
    assert(Range.getBegin().isValid() && Range.getEnd().isValid() &&
            "Invalid range in the fix-it hint.");
    assert(Range.getBegin().isFileID() && Range.getEnd().isFileID() &&
            "Only file locations supported in fix-it hints.");

    clang::tooling::Replacement Replacement(Context.getSourceManager(), Range,
                                      FixIt.CodeToInsert);
    llvm::Error Err = FileReplacements[Replacement.getFilePath()].add(Replacement);
    // FIXME: better error handling (at least, don't let other replacements be
    // applied).
    if (Err) {
      llvm::errs() << "Fix conflicts with existing fix! "
                    << llvm::toString(std::move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }

public:
  const auto& getReplacements() const { return FileReplacements; }

  explicit Dezombify2ASTVisitor(clang::ASTContext &Context, DzHelper &DzData):
    Context(Context), DzData(DzData) {}

  bool VisitCXXThisExpr(clang::CXXThisExpr *E) {

    if(E->needsDezombiefyInstrumentation()) {
      if(E->isImplicit()) {
        std::string fix = "nodecpp::safememory::dezombiefy( this )->";
        addFix(clang::FixItHint::CreateInsertion(E->getBeginLoc(), fix));
      }
      else {
        std::string fix = "nodecpp::safememory::dezombiefy( this )";
        addFix(clang::FixItHint::CreateReplacement(E->getSourceRange(), fix));
      }

    }   
    return clang::RecursiveASTVisitor<Dezombify2ASTVisitor>::VisitCXXThisExpr(E);
  }


  bool VisitDeclRefExpr(clang::DeclRefExpr *E) {
    if(E->needsDezombiefyInstrumentation()) {

      std::string fix = "nodecpp::safememory::dezombiefy( " + E->getNameInfo().getAsString() + " )";
      addFix(clang::FixItHint::CreateReplacement(E->getSourceRange(), fix));

    }   
    return clang::RecursiveASTVisitor<Dezombify2ASTVisitor>::VisitDeclRefExpr(E);
  }


  bool VisitStmt(clang::Stmt *St) {

    // auto Fix = DzData.makeFixIfNeeded(St);
    // if(Fix.hasValue())
    //   addFix(Fix.getValue());

    return clang::RecursiveASTVisitor<Dezombify2ASTVisitor>::VisitStmt(St);
  }
};

} // namespace nodecpp

#endif // NODECPP_CHECKER_DEZOMBIFY2ASTVISITOR_H

