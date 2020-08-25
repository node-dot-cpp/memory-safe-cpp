/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
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

#include "DeterminismRule.h"
#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTLambda.h"

namespace nodecpp {
namespace checker {


class RuleDASTVisitor
  : public clang::RecursiveASTVisitor<RuleDASTVisitor> {

  typedef clang::RecursiveASTVisitor<RuleDASTVisitor> Super;

  ClangTidyContext *Context;
//  MyStack St;


  /// \brief Add a diagnostic with the check's name.
  DiagnosticBuilder diag(SourceLocation Loc, StringRef Message,
                         DiagnosticIDs::Level Level = DiagnosticIDs::Error) {
    return Context->diag(DiagMsgSrc, Loc, Message, Level);
  }

  void checkTypeMembers(QualType Qt, SourceLocation Loc) {

    assert(Qt.isCanonical());
    if(Qt->getAs<RecordType>()) {
      if(!isDeterministicType(Qt, Context)) {
        diag(Loc, "(D2.1) expression type is not deterministic");
        DiagHelper Dh(Context);
        isDeterministicType(Qt, Context, Dh);
      }
    }
  }

public:

  explicit RuleDASTVisitor(ClangTidyContext *Context): Context(Context) {}

  bool TraverseDecl(Decl *D) {
    //mb: we don't traverse decls in system-headers
    //TranslationUnitDecl has an invalid location, but needs traversing anyway

    if(!D)
      return true;

    else if (isa<TranslationUnitDecl>(D))
      return Super::TraverseDecl(D);

    else if(isSystemLocation(Context, D->getLocation()))
        return true;

    else if(D->hasAttr<NodeCppNonDeterministicAttr>())
      return true;

    else if(D->hasAttr<SafeMemoryNonDeterministicAttr>())
      return true;

    else
      return Super::TraverseDecl(D);
  }

  bool VisitExplicitCastExpr(clang::ExplicitCastExpr *Expr) {
    
    if(isRawPointerType(Expr->getSubExpr()->getType().getCanonicalType())) {
      if(!isRawPointerType(Expr->getType().getCanonicalType())) {
        diag(Expr->getExprLoc(), "(D1) expression is not deterministic");
      }
    }
    return Super::VisitExplicitCastExpr(Expr);
  }


  bool VisitCXXTemporaryObjectExpr(clang::CXXTemporaryObjectExpr *Expr) {
    
    checkTypeMembers(Expr->getType().getCanonicalType(), Expr->getExprLoc());
    return Super::VisitCXXTemporaryObjectExpr(Expr);
  }

  bool VisitMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr *Expr) {
    
    checkTypeMembers(Expr->getType().getCanonicalType(), Expr->getExprLoc());
    return Super::VisitMaterializeTemporaryExpr(Expr);
  }

  bool VisitVarDecl(clang::VarDecl *Var) {

    if(isParmVarOrCatchVar(Context->getASTContext(), Var))
      return Super::VisitVarDecl(Var);

    if(Var->hasLocalStorage() || Var->isStaticLocal()) {

      auto Qt = Var->getType().getCanonicalType();
    
      if(!isDeterministicType(Qt, Context)) {
        if(Qt->getAs<RecordType>()) {
          
          diag(Var->getLocation(), "(D2.1) variable type is not deterministic");
          DiagHelper Dh(Context);
          isDeterministicType(Qt, Context, Dh);
        }
        else if (!Var->getInit()) {
          diag(Var->getLocation(), "(D2) variable type must have initializer");
        }
      }
      // else if(Qt->isConstantArrayType()) {

      //   auto Arr = cast<ConstantArrayType>(Qt->castAsArrayTypeUnsafe());
      //   auto Ie = dyn_cast_or_null<InitListExpr>(Var->getInit());

      //   auto S = Arr->getSize();
      //   if(!Ie || Ie->getNumInits() != S.getLimitedValue(UINT_MAX)) {
      //     diag(Var->getLocation(), "(D2) array type size must match the number of initializers");
      //   }
      // }
    }
    
    return Super::VisitVarDecl(Var);
  }

};

class RuleDASTConsumer : public clang::ASTConsumer {

  RuleDASTVisitor Visitor;

public:
  RuleDASTConsumer(ClangTidyContext *Context) :Visitor(Context) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

};

std::unique_ptr<clang::ASTConsumer> makeDeterminismRule(ClangTidyContext *Context) {
  return llvm::make_unique<RuleDASTConsumer>(Context);
}

} // namespace checker
} // namespace nodecpp


