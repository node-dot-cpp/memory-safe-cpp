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

#include "DezombiefyRelaxASTVisitor.h"
#include "DezombiefyRelaxAnalysis.h"

#include "clang/Sema/Sema.h"
#include "clang/Analysis/AnalysisDeclContext.h"

using namespace clang;
//using namespace clang::tooling;
using namespace llvm;
using namespace std;
using namespace nodecpp;


Optional<FixItHint> DzHelper::makeFixIfNeeded(const Stmt* St) {
  auto It = DzMap.find(St);
  if(It == DzMap.end())
    return {};

//  llvm::Twine fix;
  string Text;
  if(It->second.ThisToDZ)
    Text += "nodecpp::safememory::dezombiefy( this ); ";

  for(auto Each : It->second.VariablesToDZ) {
    Text += "nodecpp::safememory::dezombiefy( " + Each->getNameAsString() + " ); ";
  }

//  addFix(clang::FixItHint::CreateInsertion(E->getBeginLoc(), fix));
  return FixItHint::CreateInsertion(St->getBeginLoc(), Text);
}



bool DezombiefyRelaxASTVisitor::VisitFunctionDecl(FunctionDecl *D) {
  
  D->dumpColor();

  // For code in dependent contexts, we'll do this at instantiation time.
  if (D->isDependentContext())
    return true;

  const Stmt *Body = D->getBody();
  if(!Body)
    return true;

  // Construct the analysis context with the specified CFG build options.
  AnalysisDeclContext AC(/* AnalysisDeclContextManager */ nullptr, D);

  // Don't generate EH edges for CallExprs as we'd like to avoid the n^2
  // explosion for destructors that can result and the compile time hit.
  AC.getCFGBuildOptions().PruneTriviallyFalseEdges = true;
  AC.getCFGBuildOptions().AddEHEdges = false;
  AC.getCFGBuildOptions().AddInitializers = true;
  AC.getCFGBuildOptions().AddImplicitDtors = true;
  AC.getCFGBuildOptions().AddTemporaryDtors = true;
  AC.getCFGBuildOptions().AddCXXNewAllocator = false;
  AC.getCFGBuildOptions().AddCXXDefaultInitExprInCtors = true;

  AC.getCFGBuildOptions().setAllAlwaysAdd();

  if (CFG *cfg = AC.getCFG()) {
    DezombiefyRelaxAnalysisStats stats;

    cfg->dump(Context.getLangOpts(), true);

    runDezombiefyRelaxAnalysis(D, cfg, stats);

    // if (S.CollectStats && stats.NumVariablesAnalyzed > 0) {
    //   ++NumUninitAnalysisFunctions;
    //   NumUninitAnalysisVariables += stats.NumVariablesAnalyzed;
    //   NumUninitAnalysisBlockVisits += stats.NumBlockVisits;
    //   MaxUninitAnalysisVariablesPerFunction =
    //       std::max(MaxUninitAnalysisVariablesPerFunction,
    //                 stats.NumVariablesAnalyzed);
    //   MaxUninitAnalysisBlockVisitsPerFunction =
    //       std::max(MaxUninitAnalysisBlockVisitsPerFunction,
    //                 stats.NumBlockVisits);
    // }
  }



  return RecursiveASTVisitor<DezombiefyRelaxASTVisitor>::VisitFunctionDecl(D);
}

