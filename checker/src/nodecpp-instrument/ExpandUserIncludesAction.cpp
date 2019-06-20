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

#include "ExpandUserIncludesAction.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Config/config.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Rewrite/Frontend/ASTConsumers.h"
#include "clang/Rewrite/Frontend/FixItRewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Serialization/ASTReader.h"
#include "clang/Serialization/Module.h"
#include "clang/Serialization/ModuleManager.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <utility>

using namespace clang;
using namespace nodecpp;


bool ExpandUserIncludesAction::BeginSourceFileAction(CompilerInstance &CI) {



  if (!OutputStream) {
    // OutputStream = CI.createDefaultOutputFile(true, getCurrentFile());
    // if (!OutputStream)
      return false;
  }

  // auto &OS = *OutputStream;

  // If we're preprocessing a module map, start by dumping the contents of the
  // module itself before switching to the input buffer.
  // auto &Input = getCurrentInput();
  // if (Input.getKind().getFormat() == InputKind::ModuleMap) {
  //   if (Input.isFile()) {
  //     OS << "# 1 \"";
  //     OS.write_escaped(Input.getFile());
  //     OS << "\"\n";
  //   }
  //   getCurrentModule()->print(OS);
  //   OS << "#pragma clang module contents\n";
  // }

  // If we're rewriting imports, set up a listener to track when we import
  // module files.
//   if (CI.getPreprocessorOutputOpts().RewriteImports) {
//     CI.createModuleManager();
//     CI.getModuleManager()->addListener(
//         llvm::make_unique<RewriteImportsListener>(CI, OutputStream));
//   }

  return true;
}

void ExpandUserIncludesAction::ExecuteAction() {
  CompilerInstance &CI = getCompilerInstance();

  CI.getPreprocessorOutputOpts().UseLineDirectives = 1;
  // If we're rewriting imports, emit the module build output first rather
  // than switching back and forth (potentially in the middle of a line).
  // if (CI.getPreprocessorOutputOpts().RewriteImports) {
  //   std::string Buffer;
  //   llvm::raw_string_ostream OS(Buffer);

  //   RewriteUserIncludesInInput(CI.getPreprocessor(), &OS,
  //                          CI.getPreprocessorOutputOpts());

  //   (*OutputStream) << OS.str();
  // } else {
  RewriteUserIncludesInInput(CI.getPreprocessor(), OutputStream,
                           CI.getPreprocessorOutputOpts());
  // }

//  OutputStream->close();
//  OutputStream.reset();
  
}


bool ExpandRecompileAction::BeginInvocation(CompilerInstance &CI) {

  const FrontendOptions &FEOpts = CI.getFrontendOpts();
  if(Filename.empty())
    Filename = RewriteFilename(FEOpts.Inputs[0].getFile(), ".instrument");

  std::error_code EC;
  std::unique_ptr<llvm::raw_fd_ostream> OutputStream;
  OutputStream.reset(new llvm::raw_fd_ostream(Filename, EC, llvm::sys::fs::F_None));
  if (EC) {
    CI.getDiagnostics().Report(
      clang::diag::err_fe_unable_to_open_output) << Filename << EC.message();
    return false;
  }

  std::unique_ptr<FrontendAction> FixAction(new ExpandUserIncludesAction(OutputStream.get()));
  if (!FixAction->BeginSourceFile(CI, FEOpts.Inputs[0]))
    return false;
    
  FixAction->Execute();
  FixAction->EndSourceFile();

  OutputStream->close();
  OutputStream.reset();

  CI.setSourceManager(nullptr);
  CI.setFileManager(nullptr);

  CI.getDiagnosticClient().clear();

  //Create a new diagnostics
  CI.createDiagnostics();
  // auto& Diags = CI.getDiagnostics();
  // if(Diags.ownsClient()) {
  //   auto Owner = Diags.takeClient();
  //   CI.createDiagnostics(Owner.get(), true);
  // } else {
  //   CI.createDiagnostics(Diags.getClient(), false);
  // }

  PreprocessorOptions &PPOpts = CI.getPreprocessorOpts();
  // PPOpts.RemappedFiles.insert(PPOpts.RemappedFiles.end(),
  //                              RewrittenFiles.begin(), RewrittenFiles.end());
  PPOpts.RemappedFiles.emplace_back(FEOpts.Inputs[0].getFile().str(), Filename);
  PPOpts.RemappedFilesKeepOriginalName = false;


  return true;
}

std::string nodecpp::RewriteFilename(llvm::StringRef Filename, const std::string& NewSuffix) {
  SmallString<128> Path(Filename);
  llvm::sys::path::replace_extension(Path,
    NewSuffix + llvm::sys::path::extension(Path));
  return Path.str();
}

