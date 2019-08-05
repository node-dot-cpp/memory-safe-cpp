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


struct TemplateInstantiationReplacements {
  Decl* FirstInstantiation = nullptr;
  size_t Count = 0;
  bool hasError = false;
  set<Replacement> Reps;

  void add(Decl* Inst, const set<Replacement> &InstReps) {
    assert(Inst);
    if(!FirstInstantiation) {
      assert(Count == 0);
      assert(Reps.empty());

      FirstInstantiation = Inst;
      ++Count;
      Reps = InstReps;
    }
    else if(Reps == InstReps) {
      ++Count;
    }
    else if(!hasError) {
      llvm::errs() << 
        "Dezombiefy on template instantiation conflicts with previous instantiation!\n";
      FirstInstantiation->print(llvm::errs());
      Inst->print(llvm::errs());
      hasError = true;
    }

  }
};

struct TemplateInstantiationRiia {
  using StoreType = map<FunctionTemplateDecl *, map<FunctionDecl *,set<Replacement>>>;
  set<Replacement> Internal;
  set<Replacement> &External;
  StoreType &Store;
  FunctionTemplateDecl *CurrentTempl;
  FunctionDecl *CurrentInst;

  TemplateInstantiationRiia(set<Replacement> &Previous,
    StoreType &Store,
    FunctionTemplateDecl *CurrentTempl, FunctionDecl *CurrentInst)
    :External(Previous), Store(Store),
    CurrentTempl(CurrentTempl), CurrentInst(CurrentInst) {
    std::swap(External, Internal);
  }

  ~TemplateInstantiationRiia() {
    if(!External.empty()) {
      Store[CurrentTempl][CurrentInst] = External;
    }
    std::swap(Internal, External);
  }
};


class Dezombify2ASTVisitor
  : public BaseASTVisitor<Dezombify2ASTVisitor> {

  using Base = BaseASTVisitor<Dezombify2ASTVisitor>;

  /// Fixes to apply.
  Replacements FileReplacements;
  
  /// To work with template instantiations,
  /// we allow to apply several times the same replacement
  set<Replacement> TmpReplacements;

  /// To work with template instantiations
  map<FunctionTemplateDecl *, map<FunctionDecl *,set<Replacement>>> InstantiationsStore;

  void addTmpReplacement(const Replacement& Replacement) {
    TmpReplacements.insert(Replacement);
  }

  void addReplacement(const Replacement& Replacement) {
    auto Err = FileReplacements.add(Replacement);
    if (Err) {
      llvm::errs() << "Fix conflicts with existing fix! "
                    << llvm::toString(std::move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }

public:
  explicit Dezombify2ASTVisitor(ASTContext &Context):
    Base(Context) {}

  static
  pair<bool, set<Replacement>> verifyReplacements(FunctionTemplateDecl *D, const map<FunctionDecl *,set<Replacement>>& Reps) {
    pair<FunctionDecl *, set<Replacement>> Prev{false, set<Replacement>()};
    for(auto EachSpec : D->specializations()) {
      for(auto EachRed : EachSpec->redecls()) {
        if(EachRed->isTemplateInstantiation()) {
          auto ThisReps = Reps.find(EachRed);
          if(ThisReps == Reps.end()) {
            llvm::errs() << "Inconsistent dezombiefy on template instantiations\n";
            return {false, set<Replacement>()};
          }

          if(Prev.first && Prev.second != ThisReps->second) {
            llvm::errs() << "Inconsistent dezombiefy on template instantiations\n";
            return {false, set<Replacement>()};
          }

          assert(ThisReps->first);
          if(!Prev.first) {
            Prev = *ThisReps;
          }
        }
      }
    }
    return {true, Prev.second};
  }

  auto& finishReplacements() { 
    
    llvm::errs() <<
      "Dezombiefy finishReplacements!\n";
    // for each template in InstantiationsStore,
    // we must have a hit for each instantiation of such template
    // and all instantiations must have the same set of Replacements
    for(auto &EachTemp : InstantiationsStore) {
      auto R = verifyReplacements(EachTemp.first, EachTemp.second);
      if(R.first) {
        for(auto &Each : R.second)
          addReplacement(Each);
      }
    }

    for(auto &Each : TmpReplacements)
      addReplacement(Each);


    return FileReplacements;
  }

  // bool TraverseTemplateInstantiations(FunctionTemplateDecl *D) {
  //   TemplateInstantiationRiia Riia(TmpReplacements,
  //     InstantiationsStore, D, D);

  //   return Base::TraverseTemplateInstantiations(D);
  // }

  // bool TraverseTemplateInstantiations(ClassTemplateDecl *D) {
  //   TemplateInstantiationRiia Riia(TmpReplacements,
  //     InstantiationsStore, D, D);

  //   return Base::TraverseTemplateInstantiations(D);
  // }

  bool TraverseDecl(clang::Decl *D) {
    if (!D)
      return true;

    //mb: we don't traverse decls in system-headers
    if(isInSystemHeader(Context, D))
      return true;

    if(FunctionDecl *F = dyn_cast<FunctionDecl>(D)) {
      if(F->isTemplateInstantiation()) {
        TemplateInstantiationRiia Riia(TmpReplacements,
          InstantiationsStore, F->getPrimaryTemplate(), F);

        return RecursiveASTVisitor<Dezombify2ASTVisitor>::TraverseDecl(D);
      }
      else if(F->getDescribedFunctionTemplate()) {
        //mb: we don't traverse templates, only instantiations
        return true;
      }
    }
    return RecursiveASTVisitor<Dezombify2ASTVisitor>::TraverseDecl(D);
  }

  bool VisitCXXThisExpr(CXXThisExpr *E) {

    if(E->needsDezombiefyInstrumentation()) {
      if(E->isImplicit()) {
        const char *Fix = "nodecpp::safememory::dezombiefy( this )->";
        Replacement R(Context.getSourceManager(), E->getBeginLoc(), 0, Fix);
        addTmpReplacement(R);
      }
      else {
        const char *Fix = "nodecpp::safememory::dezombiefy( this )";
        Replacement R(Context.getSourceManager(), E, Fix);
        addTmpReplacement(R);
      }

    }   
    return Base::VisitCXXThisExpr(E);
  }


  bool VisitDeclRefExpr(DeclRefExpr *E) {
    if(E->needsDezombiefyInstrumentation()) {

      SmallString<64> Fix;
      Fix += "nodecpp::safememory::dezombiefy( ";
      Fix += E->getNameInfo().getAsString();
      Fix += " )";

      Replacement R(Context.getSourceManager(), E, Fix);
      addTmpReplacement(R);
    }   
    return Base::VisitDeclRefExpr(E);
  }
};

} // namespace nodecpp

#endif // NODECPP_CHECKER_DEZOMBIFY2ASTVISITOR_H

