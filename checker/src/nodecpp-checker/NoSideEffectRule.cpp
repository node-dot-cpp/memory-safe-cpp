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


#include "NoSideEffectRule.h"
#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTLambda.h"

namespace nodecpp {
namespace checker {


class NoSideEffectASTVisitor
  : public RecursiveASTVisitor<NoSideEffectASTVisitor> {

  typedef RecursiveASTVisitor<NoSideEffectASTVisitor> Super;

  ClangTidyContext *Context;

  /// \brief flags if we are currently visiting a \c [[NoSideEffect]] function or method 
  bool NoSideEffect = false;

  /// \brief riia class to help with declarations inside other declarations
  struct FlagRiia {
    bool &V;
    bool OldValue;
    FlagRiia(bool &V) :V(V) {
      OldValue = V;
      V = false;
    }
    ~FlagRiia() {
      V = OldValue;
    }
  };

  /// \brief Add a diagnostic with the check's name.
  DiagnosticBuilder diag(SourceLocation Loc, StringRef Message,
                         DiagnosticIDs::Level Level = DiagnosticIDs::Error) {
    return Context->diag(DiagMsgSrc, Loc, Message, Level);
  }
  
  CheckHelper* getCheckHelper() const { return Context->getCheckHelper(); }

public:

  explicit NoSideEffectASTVisitor(ClangTidyContext *Context): Context(Context) {}

  bool TraverseDecl(Decl *D) {
    //mb: we don't traverse decls in system-headers
    //TranslationUnitDecl has an invalid location, but needs traversing anyway

    if(!D)
      return true;

    else if (isa<TranslationUnitDecl>(D))
      return Super::TraverseDecl(D);

    else if(isSystemLocation(Context, D->getLocation()))
        return true;

    else
      return Super::TraverseDecl(D);
  }

  bool TraverseFunctionDecl(clang::FunctionDecl *D) {

    FlagRiia Riia(NoSideEffect);
    NoSideEffect = getCheckHelper()->isNoSideEffect(D);

    return Super::TraverseFunctionDecl(D);;
  }

  bool VisitCallExpr(CallExpr *E) {

    if(NoSideEffect) {

      if(!getCheckHelper()->isNoSideEffect(E->getDirectCallee())) {
        Context->diagError2(E->getExprLoc(), "no-side-effect", "function with no_side_effect attribute can call only other no side effect functions");
      }
    }

    return Super::VisitCallExpr(E);
  }

  bool VisitCXXConstructExpr(CXXConstructExpr *E) {

    if(NoSideEffect) {
      if(!E->getConstructor()->isTrivial()) {
        Context->diagError2(E->getExprLoc(), "no-side-effect", "function with no_side_effect attribute can call only other no side effect functions");
      }
    }

    return Super::VisitCXXConstructExpr(E);
  }
};


class NoSideEffectASTConsumer : public ASTConsumer {

  NoSideEffectASTVisitor Visitor;

public:
  NoSideEffectASTConsumer(ClangTidyContext *Context) :Visitor(Context) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

};

std::unique_ptr<clang::ASTConsumer> makeNoSideEffectRule(ClangTidyContext *Context) {
  return llvm::make_unique<NoSideEffectASTConsumer>(Context);
}

} // namespace checker
} // namespace nodecpp


