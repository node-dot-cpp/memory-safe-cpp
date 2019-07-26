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

#include "DezombiefyHelper.h"
#include "DezombiefyRelaxASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

namespace nodecpp {


const clang::Stmt *getIntrumentationPoint(clang::ASTContext &Context, const clang::Stmt *Ex);

const clang::Stmt *getIntrumentationPoint(clang::ASTContext &Context, const clang::VarDecl *Vd) {

  auto SList = Context.getParents(*Vd);

  auto SIt = SList.begin();

  if (SIt == SList.end()) {
    assert(false);
    return nullptr;
  }
  else if(auto D = SIt->get<clang::Stmt>())
    return getIntrumentationPoint(Context, D);
  else {
    assert(false);
    return nullptr;
  }
}



const clang::Stmt *getIntrumentationPoint(clang::ASTContext &Context, const clang::Stmt *Ex) {

  auto SList = Context.getParents(*Ex);

  auto SIt = SList.begin();

  if (SIt == SList.end()) {

    Ex->dump();
    assert(false);
    return nullptr;
  }

  if(SIt->get<clang::CompoundStmt>()) {
    //if parent is a CompoundStmt, then this node is the instrumentation point
    return Ex;
  }
  else if(auto D = SIt->get<clang::VarDecl>()) {
    //if parent is a VarDecl, keep walking up
    return getIntrumentationPoint(Context, D);
  }
  else if(auto E = SIt->get<clang::Expr>()) {
    //still an expression, keep walking up
    return getIntrumentationPoint(Context, E);
  }
  else if(auto E = SIt->get<clang::Stmt>()) {
    // now just keep walking up
    return getIntrumentationPoint(Context, E);
  }
  else {
    assert(false);
    return nullptr;

  }
}


class Dezombify1ASTVisitor
  : public clang::RecursiveASTVisitor<Dezombify1ASTVisitor> {

  clang::ASTContext &Context;
  DzHelper &DzData;
public:
  bool shouldVisitTemplateInstantiations() const { return true; }
private:
  static
  bool isParenImplicitExpr(const clang::Expr *Ex) {
    return llvm::isa<clang::ExprWithCleanups>(Ex) ||
      llvm::isa<clang::MaterializeTemporaryExpr>(Ex) ||
      llvm::isa<clang::CXXBindTemporaryExpr>(Ex) ||
      llvm::isa<clang::ImplicitCastExpr>(Ex) ||
      llvm::isa<clang::ParenExpr>(Ex);
  }

  const clang::Expr *getParentExpr(const clang::Expr *Ex) {

    auto SList = Context.getParents(*Ex);

    auto SIt = SList.begin();

    if (SIt == SList.end())
      return nullptr;

    auto P = SIt->get<clang::Expr>();
    if (!P)
      return nullptr;
    else if(isParenImplicitExpr(P))
      return getParentExpr(P);
    else
      return P;
  }

  bool hasDezombiefyParent(const clang::Expr *E) {
    auto P = getParentExpr(E);
    if(auto Ce = llvm::dyn_cast_or_null<clang::CallExpr>(P)) {
      if(auto Callee = llvm::dyn_cast_or_null<clang::UnresolvedLookupExpr>(Ce->getCallee())) {
        auto str = Callee->getNameInfo().getAsString();
      }
      else if (auto Decl = Ce->getDirectCallee())
        return Decl->getQualifiedNameAsString() == "nodecpp::safememory::dezombiefy";
    }
    return false;
  }

public:

  explicit Dezombify1ASTVisitor(clang::ASTContext &Context, DzHelper &DzData):
    Context(Context), DzData(DzData) {}

  bool TraverseDecl(clang::Decl *DeclNode) {
    if (!DeclNode) {
      return true;
    }

    //mb: we don't traverse decls in system-headers
    if (!llvm::isa<clang::TranslationUnitDecl>(DeclNode)) {

      auto &SourceManager = Context.getSourceManager();
      auto ExpansionLoc = SourceManager.getExpansionLoc(DeclNode->getLocStart());
      if (ExpansionLoc.isInvalid()) {
        return true;
      }
      if (SourceManager.isInSystemHeader(ExpansionLoc)) {
        return true;
      }
    }

    return clang::RecursiveASTVisitor<Dezombify1ASTVisitor>::TraverseDecl(DeclNode);
  }

  bool VisitParmVarDecl(clang::ParmVarDecl *D) {
//    D->dumpColor();
    return clang::RecursiveASTVisitor<Dezombify1ASTVisitor>::VisitParmVarDecl(D);
  }

  bool VisitCXXThisExpr(clang::CXXThisExpr *E) {
    if(hasDezombiefyParent(E))
      E->setDezombiefyAlreadyPresent();//TODO
    else
      E->setDezombiefyCandidate();

    const clang::Stmt *S = getIntrumentationPoint(Context, E);
    DzData.addThis(S);

    return clang::RecursiveASTVisitor<Dezombify1ASTVisitor>::VisitCXXThisExpr(E);
  }

  bool VisitDeclRefExpr(clang::DeclRefExpr *E) {
    if(IsDezombiefyCandidate(E)) {
      if(hasDezombiefyParent(E))
        E->setDezombiefyAlreadyPresent();//TODO
      else
        E->setDezombiefyCandidate();

      const clang::Stmt *S = getIntrumentationPoint(Context, E);
      DzData.addVariable(S, E->getDecl());
    }   
    return clang::RecursiveASTVisitor<Dezombify1ASTVisitor>::VisitDeclRefExpr(E);
  }
};


} // namespace nodecpp

#endif // NODECPP_CHECKER_DEZOMBIFY1ASTVISITOR_H

