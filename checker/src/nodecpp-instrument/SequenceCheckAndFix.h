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

#ifndef NODECPP_CHECKER_SEQUENCECHECKANDFIX_H
#define NODECPP_CHECKER_SEQUENCECHECKANDFIX_H


#include "clang/AST/ASTContext.h"

namespace nodecpp {

  /// Check for expressions to find attempts to dezombiefy
  /// an object unsequenced with any function call that can potencially
  /// rezombiefy it as a side-effect
struct ZombieIssuesStats {

  int Z1Count = 0;
  int Z2Count = 0;
  int Z9Count = 0;

  int UnwrapFixCount = 0;
  int Op2CallFixCount = 0;
  int UnwrapFailureCount = 0;
  int Op2CallFailureCount = 0;

  int UnfixedZ9Count = 0;
  int UnfixedZ2Count = 0;

  void printStats();

};

void sequenceCheckAndFix(clang::ASTContext &Context, bool DebugReportMode, bool SilentMode);

} // namespace nodecpp

#endif // NODECPP_CHECKER_SEQUENCECHECKANDFIX_H

