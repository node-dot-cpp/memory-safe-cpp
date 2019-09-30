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

// to swap storage on every function decl we visit.
// we will later analyze which of them are template
// instantiations of the same source code
struct TiRiia2 {
  TUChanges &Internal;
  TUChanges &External;

  TiRiia2(TUChanges &Internal, TUChanges &External)
    :Internal(Internal), External(External) {
    std::swap(External, Internal);
  }

  ~TiRiia2() {
    std::swap(Internal, External);
  }
};

struct DeduplicateHelper {
  struct InstHelper {
    FunctionDecl *TemplPattern = nullptr;
    FunctionDecl *TemplInstantiation = nullptr;
    TUChanges TemplChanges;
  };

  std::map<FunctionDecl *, InstHelper> Data;

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

  bool add(FunctionDecl *TemplPattern, FunctionDecl *TemplInstantiation,
    TUChanges TemplChanges, clang::DiagnosticsEngine &DE,
    const clang::LangOptions &Lang) {

      auto It2 = Data.find(TemplPattern);
      if(It2 == Data.end()) {
        // is the first time we hit this pattern, just add it.
        Data[TemplPattern] = InstHelper{TemplPattern, TemplInstantiation, TemplChanges};
        return true;
      }
      else {
        auto D = It2->second.TemplPattern;
        auto I1 = It2->second.TemplInstantiation;
        auto &Changes1 = It2->second.TemplChanges;
        auto I2 = TemplInstantiation;
        auto &Changes2 = TemplChanges;
        //verify it is equal
        return verifyReplacements2(D, I1, Changes1, I2, Changes2, DE, Lang);
      }
    }

    auto begin() const { return Data.begin(); }
    auto end() const { return Data.end(); }
};


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
//  TUChanges TmpReplacements;
  using StoreType = std::map<FunctionDecl *, TUChanges>;

  /// To work with template instantiations
  StoreType Store;

  // instantiations to template pattern map 
  // don't use map, so order is fixed
  std::vector<std::pair<FunctionDecl *, FunctionDecl *>> Inst2Templ;

  bool IsTemplate = false;


public:
  using Base = clang::RecursiveASTVisitor<T>;

  explicit BaseASTVisitor(clang::ASTContext &Context):
    Context(Context) {}

  //template inst are implicits, so we actually need both
  bool shouldVisitTemplateInstantiations() const { return true; }

  void addReplacement(const CodeChange& Replacement) {
    auto Err = FileReplacements.add(Context.getSourceManager(), Replacement);
    if (Err) {
      llvm::errs() << "Fix conflicts with existing fix! "
                    << llvm::toString(std::move(Err)) << "\n";
      assert(false && "Fix conflicts with existing fix!");
    }
  }

  void addReplacement(const FileChanges& Replacements) {
    for(auto& Each : Replacements) {
        addReplacement(Each);
    }
  }

  void addReplacement(const TUChanges& Replacements) {
    for(auto& Each : Replacements) {
        addReplacement(Each.second);
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
    DeduplicateHelper Helper;

    for(auto& Each : Inst2Templ) {
      //first, the second is the template pattern,
      //if we have it in the set, we drop it
      //since instantiations put the rules
      auto ItPatt = Store.find(Each.second);
      if(ItPatt != Store.end()) {
        Store.erase(ItPatt);
      }

      //now we look up the instantiation
      auto It = Store.find(Each.first);
      assert(It != Store.end());

      Helper.add(Each.second, It->first, It->second, DE, Lang);

      //now we remove it from instantiation store
      Store.erase(It);
    }


    //Now things in Result are clean, and only one time for each template
    for(auto &Each : Helper) {
      addReplacement(Each.second.TemplChanges);
    }

    //anything left in instantiation store, is not really template related
    //just add
    for(auto &Each : Store) {
      addReplacement(Each.second);
    }

    return FileReplacements;
  }

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

      auto &FuncStore = Store[F];
      TiRiia2 Riia(FileReplacements, FuncStore);

      return clang::RecursiveASTVisitor<T>::TraverseDecl(D);
    }

    return clang::RecursiveASTVisitor<T>::TraverseDecl(D);
  }
};

} // namespace nodecpp

#endif // NODECPP_INSTRUMENT_BASEASTVISITOR_H

