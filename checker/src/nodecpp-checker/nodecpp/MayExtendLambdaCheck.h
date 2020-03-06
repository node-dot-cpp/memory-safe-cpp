/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- MayExtendLambdaCheck.h - clang-tidy---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef NODECPP_CHECKER_NODECPP_MAYEXTENDLAMBDACHECK_H
#define NODECPP_CHECKER_NODECPP_MAYEXTENDLAMBDACHECK_H

#include "../ClangTidy.h"

namespace nodecpp {
namespace checker {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/nodecpp-may-extend-lambda.html
class MayExtendLambdaCheck : public ClangTidyCheck {
public:
  MayExtendLambdaCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;

  bool checkLambda(const LambdaExpr *Lamb, bool IsCompositeOfThis);

  /// returns \c true if the expression was checked as a lambda 
  /// (either positively or negatively)
  bool tryCheckAsLambda(const Expr *Ex,  bool IsCompositeOfThis);

  static bool isCompositeOfThis(const Expr *Ex);

  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace checker
} // namespace nodecpp

#endif // NODECPP_CHECKER_NODECPP_MAYEXTENDLAMBDACHECK_H
