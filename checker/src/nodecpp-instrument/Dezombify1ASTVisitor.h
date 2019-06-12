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

#ifndef NODECPP_CHECKER_DEZOMBIFY1ASTVISITOR_H
#define NODECPP_CHECKER_DEZOMBIFY1ASTVISITOR_H


#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;

namespace nodecpp {

class Dezombify1ASTVisitor
  : public RecursiveASTVisitor<Dezombify1ASTVisitor> {

  ASTContext &Context;
public:
  bool shouldVisitTemplateInstantiations() const { return true; }
private:
  static
  bool isParenImplicitExpr(const Expr *Ex) {
    return isa<ExprWithCleanups>(Ex) ||
      isa<MaterializeTemporaryExpr>(Ex) ||
      isa<CXXBindTemporaryExpr>(Ex) ||
      isa<ImplicitCastExpr>(Ex) ||
      isa<ParenExpr>(Ex);
  }

  const Expr *getParentExpr(const Expr *Ex) {

    auto SList = Context.getParents(*Ex);

    auto SIt = SList.begin();

    if (SIt == SList.end())
      return nullptr;

    auto P = SIt->get<Expr>();
    if (!P)
      return nullptr;
    else if(isParenImplicitExpr(P))
      return getParentExpr(P);
    else
      return P;
  }

  bool hasDezombiefyParent(const Expr *E) {
    auto P = getParentExpr(E);
    if(auto Ce = dyn_cast_or_null<CallExpr>(P)) {
      if(auto Callee = dyn_cast_or_null<UnresolvedLookupExpr>(Ce->getCallee())) {
        auto str = Callee->getNameInfo().getAsString();
      }
      else if (auto Decl = Ce->getDirectCallee())
        return Decl->getQualifiedNameAsString() == "nodecpp::safememory::dezombiefy";
    }
    return false;
  }

public:

  explicit Dezombify1ASTVisitor(ASTContext &Context): Context(Context) {}

  bool TraverseDecl(Decl *DeclNode) {
    if (!DeclNode) {
      return true;
    }

    //mb: we don't traverse decls in system-headers
    if (!isa<TranslationUnitDecl>(DeclNode)) {

      auto &SourceManager = Context.getSourceManager();
      auto ExpansionLoc = SourceManager.getExpansionLoc(DeclNode->getLocStart());
      if (ExpansionLoc.isInvalid()) {
        return true;
      }
      if (SourceManager.isInSystemHeader(ExpansionLoc)) {
        return true;
      }
    }

    return RecursiveASTVisitor<Dezombify1ASTVisitor>::TraverseDecl(DeclNode);
  }

  bool VisitParmVarDecl(ParmVarDecl *D) {
//    D->dumpColor();
    return RecursiveASTVisitor<Dezombify1ASTVisitor>::VisitParmVarDecl(D);
  }

  bool VisitCXXThisExpr(CXXThisExpr *E) {
    if(hasDezombiefyParent(E))
      E->setDezombiefyAlreadyPresent();//TODO
    else
      E->setDezombiefyCandidate();

    return RecursiveASTVisitor<Dezombify1ASTVisitor>::VisitCXXThisExpr(E);
  }

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    if(auto D = E->getDecl()) {
      auto Qt = D->getType().getCanonicalType();
//      auto T = Qt.getTypePtr();
      if(Qt->isReferenceType() || Qt->isPointerType()) {

        if(hasDezombiefyParent(E))
          E->setDezombiefyAlreadyPresent();//TODO
        else
          E->setDezombiefyCandidate();

      }
    }   
    return RecursiveASTVisitor<Dezombify1ASTVisitor>::VisitDeclRefExpr(E);
  }
};


} // namespace nodecpp

#endif // NODECPP_CHECKER_DEZOMBIFY1ASTVISITOR_H

