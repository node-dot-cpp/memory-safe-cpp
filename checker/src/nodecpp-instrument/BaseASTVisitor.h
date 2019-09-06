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

#ifndef NODECPP_INSTRUMENT_BASEASTVISITOR_H
#define NODECPP_INSTRUMENT_BASEASTVISITOR_H

#include "DezombiefyHelper.h"

#include "CodeChange.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Format/Format.h"


namespace nodecpp {

using namespace std;
using namespace llvm;
using namespace clang;
using namespace clang::tooling;


struct TiRiia {
  using StoreType = std::map<FunctionDecl *, TUChanges>;
//  using StoreType = std::map<FunctionDecl *, std::map<FunctionDecl *, TUChanges>>;
  TUChanges Internal;
  TUChanges &External;
  StoreType &Store;
//  FunctionDecl *CurrentTempl;
  FunctionDecl *CurrentInst;

  TiRiia(TUChanges &Previous,
    StoreType &Store, FunctionDecl *CurrentInst)
    :External(Previous), Store(Store),
     CurrentInst(CurrentInst) {
    std::swap(External, Internal);
//    assert(CurrentTempl);
    assert(CurrentInst);
  }

  ~TiRiia() {
    Store[CurrentInst] = External;
    std::swap(Internal, External);
  }
};


inline
bool isInSystemHeader(clang::ASTContext &Context, clang::Decl *D) {
  
  if (!llvm::isa<clang::TranslationUnitDecl>(D)) {

    auto &SourceManager = Context.getSourceManager();
    auto ExpansionLoc = SourceManager.getExpansionLoc(D->getLocStart());
    if (ExpansionLoc.isInvalid()) {
      return true;
    }
    if (SourceManager.isInSystemHeader(ExpansionLoc)) {
      return true;
    }
  }
  return false;
}

template<class T>
class BaseASTVisitor
  : public clang::RecursiveASTVisitor<T> {
protected:
  clang::ASTContext &Context;
private:
  /// Fixes to apply.
  TUChanges FileReplacements;
  
  /// To work with template instantiations,
  /// we allow to apply several times the same replacement
  TUChanges TmpReplacements;

  /// To work with template instantiations
  TiRiia::StoreType InstantiationsStore;

  // instantiations to template pattern map 
  // don't use map, so order is fixed
  std::vector<std::pair<FunctionDecl *, FunctionDecl *>> Inst2Templ;

  void addReplacement(const CodeChange& Replacement) {
    auto Err = FileReplacements.add(Context.getSourceManager(), Replacement);
    if (Err) {
      llvm::errs() << "Fix conflicts with existing fix! "
                    << llvm::toString(std::move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }

  static
  bool verifyReplacements2(clang::FunctionDecl *D, clang::FunctionDecl *I1, 
    const TUChanges& Changes1, clang::FunctionDecl *I2, const TUChanges& Changes2,
    clang::DiagnosticsEngine &DE, const clang::LangOptions &Lang) {
    
      if(Changes1 == Changes2)
        return true;


    unsigned ID = DE.getDiagnosticIDs()->getCustomDiagID(
      clang::DiagnosticIDs::Error,
      "Template funcion '%0' has inconsistent dezombiefy requirements [nodecpp-dezombiefy]");

    DE.Report(D->getLocation(), ID) << D->getName();

    unsigned ID2 = DE.getDiagnosticIDs()->getCustomDiagID(
      clang::DiagnosticIDs::Note,
      "Intantiated as '%0' here");


    std::string Str;
    llvm::raw_string_ostream Os(Str);
    // llvm::errs() << "Inconsistent dezombiefy on template instantiations\n";
    // LangOptions L;
    clang::PrintingPolicy P(Lang);
    P.TerseOutput = true;
    P.FullyQualifiedName = true;
    I1->print(Os, P, 0, true);

    DE.Report(I1->getPointOfInstantiation(), ID2) << Os.str();

    Os.str().clear();

    I2->print(Os, P, 0, true);
    DE.Report(I2->getPointOfInstantiation(), ID2) << Os.str();

    return false;
  }

public:
  using Base = clang::RecursiveASTVisitor<T>;

  explicit BaseASTVisitor(clang::ASTContext &Context):
    Context(Context) {}

  //template inst are implicits, so we actually need both
  bool shouldVisitTemplateInstantiations() const { return true; }

  void addTmpReplacement(const CodeChange& Replacement) {
    auto Err = TmpReplacements.add(Context.getSourceManager(), Replacement);
    if (Err) {
      llvm::errs() << "Fix conflicts with existing fix! "
                    << llvm::toString(std::move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }

  }

  void addTmpReplacement(const FileChanges& Replacements) {
    for(auto& Each : Replacements) {
        addTmpReplacement(Each);
    }
  }
  
  auto& finishReplacements() {
    return finishReplacements(Context.getDiagnostics(), Context.getLangOpts());
  } 

  auto& finishReplacements(clang::DiagnosticsEngine &DE, const clang::LangOptions &Lang) { 
    
    // llvm::errs() <<
    //   "Dezombiefy finishReplacements!\n";
    // for each template in InstantiationsStore,
    // we must have check all instantiation of such template
    // and they must have the same set of Replacements
    TiRiia::StoreType Result;
    std::map<FunctionDecl *, FunctionDecl *> ResultInst;

    for(auto& Each : Inst2Templ) {
      //first, the second is the template pattern,
      //if we have it in the set, we drop it
      //since instantiations put the rules
      auto ItPatt = InstantiationsStore.find(Each.second);
      if(ItPatt != InstantiationsStore.end()) {
        InstantiationsStore.erase(ItPatt);
      }

      //now we look up the instantiation
      auto It = InstantiationsStore.find(Each.first);
      assert(It != InstantiationsStore.end());
      auto It2 = Result.find(Each.second);
      if(It2 == Result.end()) {
        // is the first time we hit this pattern, just add it.
        Result[Each.second] = It->second;
        ResultInst[Each.second] = Each.first;
      }
      else {
        auto D = Each.second;
        auto I1 = ResultInst[Each.second];
        auto &Changes1 = Result[Each.second];
        auto I2 = Each.first;
        auto &Changes2 = It->second;
        //verify it is equal
        verifyReplacements2(D, I1, Changes1, I2, Changes2, DE, Lang);
      }

      //now we remove it from instantiation store
      InstantiationsStore.erase(It);
    }


    //Now things in Result are clean, and only one time for each template
    for(auto &Each : Result) {
      for(auto &Each2 : Each.second) {
        for(auto &Each3 : Each2.second) {
          addReplacement(Each3);
        }
      }
    }

    //anything left in instantiation store, is not template related
    //just add
    for(auto &Each : InstantiationsStore) {
      for(auto &Each2 : Each.second) {
        for(auto &Each3 : Each2.second) {
          addReplacement(Each3);
        }
      }
    }

    return FileReplacements;
  }


  // auto& finishReplacements(clang::DiagnosticsEngine &DE, const clang::LangOptions &Lang) { 
    
  //   // llvm::errs() <<
  //   //   "Dezombiefy finishReplacements!\n";
  //   // for each template in InstantiationsStore,
  //   // we must have check all instantiation of such template
  //   // and they must have the same set of Replacements
  //   for(auto &EachTemp : InstantiationsStore) {
  //     auto R = verifyReplacements(EachTemp.first, EachTemp.second, DE, Lang);
  //     if(R.first) {
  //       for(auto &Each : R.second) {
  //         for(auto &Each2 : Each.second) {
  //           addReplacement(Each2);
  //         }
  //       }
  //     }
  //   }

  //   for(auto &Each : TmpReplacements) {
  //     for(auto &Each2 : Each.second) {
  //       addReplacement(Each2);
  //     }
  //   }


  //   return FileReplacements;
  // }

  bool TraverseDecl(clang::Decl *D) {
    if (!D)
      return true;

    //mb: we don't traverse decls in system-headers
    if(isInSystemHeader(Context, D))
      return true;


    // Traverse* are type specific and don't
    // walk up, like Visit* do, so we check it here
    if(auto F = dyn_cast<clang::FunctionDecl>(D)) {
      if(auto P = F->getTemplateInstantiationPattern()) {
        Inst2Templ.emplace_back(F, P);
      }

      TiRiia Riia(TmpReplacements, InstantiationsStore, F);

      return clang::RecursiveASTVisitor<T>::TraverseDecl(D);
    }

    return clang::RecursiveASTVisitor<T>::TraverseDecl(D);
  }

  // bool TraverseFunctionTemplateDecl(clang::FunctionTemplateDecl *D) {
  
  //   return clang::RecursiveASTVisitor<T>::TraverseFunctionTemplateDecl(D);
  // }

  // bool TraverseCXXMethodDecl(clang::CXXMethodDecl *D) {

  //   return clang::RecursiveASTVisitor<T>::TraverseCXXMethodDecl(D);
  // }

  // bool TraverseFunctionDecl(clang::FunctionDecl *D) {
    
  //   //checks at TraverseDecl are already executed
  //   assert(D);
  //   assert(!isInSystemHeader(Context, D));

  //   if(D->getDescribedFunctionTemplate()) {
  //     return true;//template itself is not analyzed, only its intantatiations
  //   }
  //   else if(auto P = D->getTemplateInstantiationPattern()) {
  //     TiRiia Riia(TmpReplacements, InstantiationsStore, P, D);

  //     return clang::RecursiveASTVisitor<T>::TraverseFunctionDecl(D);
  //   }
  //   else {
  //     return clang::RecursiveASTVisitor<T>::TraverseFunctionDecl(D);
  //   }
  // }

};

} // namespace nodecpp

#endif // NODECPP_INSTRUMENT_BASEASTVISITOR_H

