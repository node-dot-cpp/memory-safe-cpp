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

#ifndef NODECPP_CHECKER_CHECKERASTVISITOR_H
#define NODECPP_CHECKER_CHECKERASTVISITOR_H

#include "FlagRiia.h"
#include "nodecpp/NakedPtrHelper.h"
#include "clang/AST/RecursiveASTVisitor.h"


namespace nodecpp {
namespace checker {

class ClangTidyContext;

enum class CheckerKind {All, Safety, Determinism};

template<class T, CheckerKind KIND>
class BaseCheckerASTVisitor : public RecursiveASTVisitor<T> {

protected:
  typedef RecursiveASTVisitor<T> Super;
  ClangTidyContext *Context = nullptr;

  /// \brief flags if we are currently visiting a \c [[check_as_user_code]] namespace 
  bool CheckAsUserCode = false;

public:
//  bool shouldVisitImplicitCode() const { return true; }
  bool shouldVisitTemplateInstantiations() const { return true; }  

  explicit BaseCheckerASTVisitor(ClangTidyContext *Context): Context(Context) {}

  bool TraverseDecl(Decl *D) {

    if(!D)
      return true;
    else if (isa<TranslationUnitDecl>(D)) // has an invalid location
      return Super::TraverseDecl(D);
    else if (auto Ns = dyn_cast<NamespaceDecl>(D)) {
      if(Ns->hasAttr<SafeMemoryCheckAsUserCodeAttr>()) {
        FlagRiia R(CheckAsUserCode);
        return Super::TraverseDecl(D);
      }
      else if (KIND == CheckerKind::Safety &&
        (Ns->hasAttr<SafeMemoryMemoryUnsafeAttr>() ||
        Ns->hasAttr<NodeCppMemoryUnsafeAttr>()))
        return true;
      else if (KIND == CheckerKind::Determinism &&
        (Ns->hasAttr<SafeMemoryNonDeterministicAttr>() ||
        Ns->hasAttr<NodeCppNonDeterministicAttr>()))
        return true;
      else //namespaces are traversed independant of location
        return Super::TraverseDecl(D);
    }
    else if(CheckAsUserCode) // takes precedence over location
      return Super::TraverseDecl(D);
    else if(isSystemLocation(Context, D->getLocation()))
      return true;
    else      
      return Super::TraverseDecl(D);
  }
};


template<class T>
using CheckerASTVisitor = BaseCheckerASTVisitor<T, CheckerKind::All>;

template<class T>
using SafetyASTVisitor = BaseCheckerASTVisitor<T, CheckerKind::Safety>;

template<class T>
using DeterminismASTVisitor = BaseCheckerASTVisitor<T, CheckerKind::Determinism>;


} // namespace checker
} // namespace nodecpp

#endif
