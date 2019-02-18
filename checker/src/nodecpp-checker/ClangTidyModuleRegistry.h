//===--- ClangTidyModuleRegistry.h - clang-tidy -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef NODECPP_CHECKER_CLANGTIDYMODULEREGISTRY_H
#define NODECPP_CHECKER_CLANGTIDYMODULEREGISTRY_H

#include "ClangTidyModule.h"
#include "llvm/Support/Registry.h"

namespace clang {
namespace tidy {

typedef llvm::Registry<ClangTidyModule> ClangTidyModuleRegistry;

} // end namespace tidy
} // end namespace clang

#endif // NODECPP_CHECKER_CLANGTIDYMODULEREGISTRY_H
