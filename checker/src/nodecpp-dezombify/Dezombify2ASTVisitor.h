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


#include "nodecpp/NakedPtrHelper.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Basic/Diagnostic.h"

using namespace clang;

namespace nodecpp {

class Dezombify2ASTVisitor
  : public clang::RecursiveASTVisitor<Dezombify2ASTVisitor> {

  clang::ASTContext &Context;
  /// Fixes to apply, grouped by file path.
  llvm::StringMap<tooling::Replacements> Fix;  


  void addFix(const SourceManager& Manager, const FixItHint& FixIt) {

    CharSourceRange Range = FixIt.RemoveRange;
    assert(Range.getBegin().isValid() && Range.getEnd().isValid() &&
            "Invalid range in the fix-it hint.");
    assert(Range.getBegin().isFileID() && Range.getEnd().isFileID() &&
            "Only file locations supported in fix-it hints.");

    tooling::Replacement Replacement(Manager, Range,
                                      FixIt.CodeToInsert);
    llvm::Error Err = Fix[Replacement.getFilePath()].add(Replacement);
    // FIXME: better error handling (at least, don't let other replacements be
    // applied).
    if (Err) {
      llvm::errs() << "Fix conflicts with existing fix! "
                    << llvm::toString(std::move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }

public:

  explicit Dezombify2ASTVisitor(clang::ASTContext &Context): Context(Context) {}


  bool VisitDeclRefExpr(clang::DeclRefExpr *E) {
    if(E && E->isDezombifyReallyNeeded()) {

      std::string fix = "dezombify( " + E->getNameInfo().getAsString() + " )";
      addFix(Context.getSourceManager(), FixItHint::CreateReplacement(E->getSourceRange(), fix));

    }   
    return true;
  }

};

} // namespace nodecpp

#endif // NODECPP_CHECKER_DEZOMBIFY2ASTVISITOR_H

