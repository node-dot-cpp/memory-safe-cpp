/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
* All rights reserved.
*
* Initial version copied from include/clang/Analysis/UninitializedValues.h
*
* -------------------------------------------------------------------------------*/

//=- UninitializedValues.h - Finding uses of uninitialized values -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines APIs for invoking and reported uninitialized values
// warnings.
//
//===----------------------------------------------------------------------===//

#ifndef NODECPP_INSTRUMENT_DEZOMBIEFYRELAXANALYSIS_H
#define NODECPP_INSTRUMENT_DEZOMBIEFYRELAXANALYSIS_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/SmallVector.h"

namespace clang {

class AnalysisDeclContext;
class CFG;
class DeclContext;
class Expr;
class Stmt;
class VarDecl;

} // namespace clang

namespace nodecpp {

/// A use of a variable, which might be uninitialized.
class UninitUse {
public:
  struct Branch {
    const clang::Stmt *Terminator;
    unsigned Output;
  };

private:
  /// The expression which uses this variable.
  const clang::Expr *User;

  /// Is this use uninitialized whenever the function is called?
  bool UninitAfterCall = false;

  /// Is this use uninitialized whenever the variable declaration is reached?
  bool UninitAfterDecl = false;

  /// Does this use always see an uninitialized value?
  bool AlwaysUninit;

  /// This use is always uninitialized if it occurs after any of these branches
  /// is taken.
  llvm::SmallVector<Branch, 2> UninitBranches;

public:
  UninitUse(const clang::Expr *User, bool AlwaysUninit)
      : User(User), AlwaysUninit(AlwaysUninit) {}

  void addUninitBranch(Branch B) {
    UninitBranches.push_back(B);
  }

  void setUninitAfterCall() { UninitAfterCall = true; }
  void setUninitAfterDecl() { UninitAfterDecl = true; }

  /// Get the expression containing the uninitialized use.
  const clang::Expr *getUser() const { return User; }

  /// The kind of uninitialized use.
  enum Kind {
    /// The use might be uninitialized.
    Maybe,

    /// The use is uninitialized whenever a certain branch is taken.
    Sometimes,

    /// The use is uninitialized the first time it is reached after we reach
    /// the variable's declaration.
    AfterDecl,

    /// The use is uninitialized the first time it is reached after the function
    /// is called.
    AfterCall,

    /// The use is always uninitialized.
    Always
  };

  /// Get the kind of uninitialized use.
  Kind getKind() const {
    return AlwaysUninit ? Always :
           UninitAfterCall ? AfterCall :
           UninitAfterDecl ? AfterDecl :
           !branch_empty() ? Sometimes : Maybe;
  }

  using branch_iterator = llvm::SmallVectorImpl<Branch>::const_iterator;

  /// Branches which inevitably result in the variable being used uninitialized.
  branch_iterator branch_begin() const { return UninitBranches.begin(); }
  branch_iterator branch_end() const { return UninitBranches.end(); }
  bool branch_empty() const { return UninitBranches.empty(); }
};

class UninitVariablesHandler {
public:
  UninitVariablesHandler() = default;
  virtual ~UninitVariablesHandler();

  /// Called when the uninitialized variable is used at the given expression.
  virtual void handleUseOfUninitVariable(const clang::VarDecl *vd,
                                         const UninitUse &use) {}

  /// Called when the uninitialized variable analysis detects the
  /// idiom 'int x = x'.  All other uses of 'x' within the initializer
  /// are handled by handleUseOfUninitVariable.
  virtual void handleSelfInit(const clang::VarDecl *vd) {}
};

struct UninitVariablesAnalysisStats {
  unsigned NumVariablesAnalyzed = 0;
  unsigned NumBlockVisits = 0;
};

void runDezombiefyRelaxAnalysis(const clang::DeclContext &dc, const clang::CFG &cfg,
                                       clang::AnalysisDeclContext &ac,
                                       UninitVariablesHandler &handler,
                                       UninitVariablesAnalysisStats &stats);

} // namespace nodecpp

#endif // NODECPP_INSTRUMENT_DEZOMBIEFYRELAXANALYSIS_H
