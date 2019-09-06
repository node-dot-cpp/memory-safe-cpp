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

#include "DezombiefyHelper.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Format/Format.h"


namespace nodecpp {

using namespace std;
using namespace llvm;
using namespace clang;
using namespace clang::tooling;


class Dezombify2ASTVisitor
  : public BaseASTVisitor<Dezombify2ASTVisitor> {

  using Base = BaseASTVisitor<Dezombify2ASTVisitor>;

 
public:
  explicit Dezombify2ASTVisitor(ASTContext &Context):
    Base(Context) {}


  bool VisitCXXThisExpr(CXXThisExpr *E) {

    if(E->needsDezombiefyInstrumentation()) {
      if(E->isImplicit()) {
        const char *Fix = "nodecpp::safememory::dezombiefy( this )->";
        addReplacement(CodeChange::makeInsertLeft(
          Context.getSourceManager(), E->getBeginLoc(), Fix));
      }
      else {
        const char *Fix = "nodecpp::safememory::dezombiefy( this )";
//        Replacement R(Context.getSourceManager(), E, Fix);
        addReplacement(CodeChange::makeReplace(
          Context.getSourceManager(), E->getSourceRange(), Fix));
      }

    }   
    return Base::VisitCXXThisExpr(E);
  }


  bool VisitDeclRefExpr(DeclRefExpr *E) {
    if(E->needsDezombiefyInstrumentation()) {
//      E->dumpColor();

      SmallString<64> Fix;
      Fix += "nodecpp::safememory::dezombiefy( ";
      Fix += E->getNameInfo().getAsString();
      Fix += " )";

      // Replacement R(Context.getSourceManager(), E, Fix);
      // addTmpReplacement(R);
      addReplacement(CodeChange::makeReplace(
        Context.getSourceManager(), E->getSourceRange(), Fix));
    }   
    return Base::VisitDeclRefExpr(E);
  }
};

} // namespace nodecpp

#endif // NODECPP_CHECKER_DEZOMBIFY2ASTVISITOR_H

