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

#ifndef NODECPP_INSTRUMENT_EXPANDUSERINCLUDESACTION_H
#define NODECPP_INSTRUMENT_EXPANDUSERINCLUDESACTION_H

#include "clang/Frontend/FrontendAction.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace clang {
    class Preprocessor;
    class PreprocessorOutputOptions;
}

namespace nodecpp {

class ExpandUserIncludesAction : public PreprocessorFrontendAction {
  llvm::raw_ostream *OutputStream;
//  class RewriteImportsListener;
protected:
  bool BeginSourceFileAction(clang::CompilerInstance &CI) override;
  void ExecuteAction() override;

public:
  ExpandUserIncludesAction(llvm::raw_ostream *OutputStream)
    : OutputStream(OutputStream) {}
  ExpandUserIncludesAction(): OutputStream(nullptr) {}
};

class ExpandRecompileAction : public WrapperFrontendAction {
  std::string Filename;
public:

  ExpandRecompileAction(std::unique_ptr<FrontendAction> WrappedAction, const std::string& Filename)
    : WrapperFrontendAction(std::move(WrappedAction)), Filename(Filename) {}

protected:
  bool BeginInvocation(CompilerInstance &CI) override;
};

void RewriteUserIncludesInInput(clang::Preprocessor &PP, llvm::raw_ostream *OS,
                            const clang::PreprocessorOutputOptions &Opts);

std::string RewriteFilename(llvm::StringRef Filename, const std::string& NewSuffix);

} // namespace nodecpp



#endif // NODECPP_INSTRUMENT_EXPANDUSERINCLUDESACTION_H
