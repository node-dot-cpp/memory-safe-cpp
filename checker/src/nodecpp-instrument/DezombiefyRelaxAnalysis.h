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

} // namespace clang

namespace nodecpp {

struct DezombiefyRelaxAnalysisStats {
//  unsigned NumVariablesAnalyzed = 0;
  unsigned NumBlockVisits = 0;
};

void runDezombiefyRelaxAnalysis(const clang::DeclContext *dc, const clang::CFG *cfg,
                                       DezombiefyRelaxAnalysisStats &stats);

} // namespace nodecpp

#endif // NODECPP_INSTRUMENT_DEZOMBIEFYRELAXANALYSIS_H
