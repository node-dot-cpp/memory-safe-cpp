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

#ifndef NODECPP_CHECKER_DEZOMBIEFYRELAXASTVISITOR_H
#define NODECPP_CHECKER_DEZOMBIEFYRELAXASTVISITOR_H

#include "DezombiefyHelper.h"

#include "BaseASTVisitor.h"
#include "DezombiefyRelaxAnalysis.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

namespace nodecpp {


class DzHelper {
  struct ToBeDezombiefied {
      llvm::SmallVector<const clang::ValueDecl*, 2> VariablesToDZ;
      bool ThisToDZ = false;

      ToBeDezombiefied(const clang::ValueDecl* Dc) :VariablesToDZ({Dc}) {}

      ToBeDezombiefied(bool Th) :ThisToDZ(Th) {}

  };

  llvm::DenseMap<const clang::Stmt*, ToBeDezombiefied> DzMap;

  public:

  void addVariable(const clang::Stmt *St, const clang::ValueDecl *Dc) {
    auto R = DzMap.try_emplace(St, Dc);
    if(!R.second) {
      auto& Vec = R.first->second.VariablesToDZ;
      for(auto Each : Vec) {
        if(Each == Dc) //already here
          return;
      }
      Vec.push_back(Dc);
    }
  }

  void addThis(const clang::Stmt* St) {
    auto R = DzMap.try_emplace(St, true);
    if(!R.second) {
      R.first->second.ThisToDZ = true;
    }
  }

};

class DezombiefyRelaxASTVisitor
  : public BaseASTVisitor<DezombiefyRelaxASTVisitor> {

  using Base = BaseASTVisitor<DezombiefyRelaxASTVisitor>;

  DzHelper &DzData;
public:
  explicit DezombiefyRelaxASTVisitor(clang::ASTContext &Context, DzHelper &DzData):
    Base(Context), DzData(DzData) {}

  bool VisitFunctionDecl(clang::FunctionDecl *D) {

    // For code in dependent contexts, we'll do this at instantiation time.
    if (D->isDependentContext())
      return true;

    if(!D->getBody())
      return true;

    runDezombiefyRelaxAnalysis(D);

    return Base::VisitFunctionDecl(D);
  }
};

//void dezombiefyRelax(clang::ASTContext &Context);

} // namespace nodecpp

#endif // NODECPP_CHECKER_DEZOMBIEFYRELAXASTVISITOR_H

