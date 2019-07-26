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

#include "Dezombify1ASTVisitor.h"
#include "Dezombify2ASTVisitor.h"
#include "ExpandUserIncludesAction.h"
#include "DezombiefyRelaxASTVisitor.h"
#include "DeunsequenceASTVisitor.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static cl::OptionCategory NodecppInstrumentCategory("nodecpp-instrument options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");

//static cl::opt<bool> Help("h", cl::desc("Alias for -help"), cl::Hidden);

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"), cl::cat(NodecppInstrumentCategory));

// static cl::opt<bool>
// Dump("dump", cl::desc("Generate an idl tree and dump it.\n"),
//     cl::cat(NodecppInstrumentCategory));

namespace nodecpp {

void overwriteChangedFiles(ASTContext &Context, const StringMap<Replacements> &FileReplacements) {

  if(!FileReplacements.empty()) {
    clang::Rewriter Rewrite(Context.getSourceManager(), Context.getLangOpts());
    for (const auto &FileAndReplacements : FileReplacements) {
      llvm::StringRef File = FileAndReplacements.first();
      llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> Buffer =
          Context.getSourceManager().getFileManager().getBufferForFile(File);
      if (!Buffer) {
        llvm::errs() << "Can't get buffer for file " << File << ": "
                      << Buffer.getError().message() << "\n";
        // FIXME: Maybe don't apply fixes for other files as well.
        continue;
      }
      llvm::StringRef Code = Buffer.get()->getBuffer();
      // auto Style = format::getStyle(
      //     *Context.getOptionsForFile(File).FormatStyle, File, "none");
      auto Style = clang::format::getStyle("none", File, "none");
      if (!Style) {
        llvm::errs() << llvm::toString(Style.takeError()) << "\n";
        continue;
      }
      llvm::Expected<clang::tooling::Replacements> Replacements =
          clang::format::cleanupAroundReplacements(Code, FileAndReplacements.second,
                                            *Style);
      if (!Replacements) {
        llvm::errs() << llvm::toString(Replacements.takeError()) << "\n";
        continue;
      }
      if (llvm::Expected<clang::tooling::Replacements> FormattedReplacements =
              clang::format::formatReplacements(Code, *Replacements, *Style)) {
        Replacements = std::move(FormattedReplacements);
        if (!Replacements)
          llvm_unreachable("!Replacements");
      } else {
        llvm::errs() << llvm::toString(FormattedReplacements.takeError())
                      << ". Skipping formatting.\n";
      }
      if (!clang::tooling::applyAllReplacements(Replacements.get(), Rewrite)) {
        llvm::errs() << "Can't apply replacements for file " << File << "\n";
      }
    }
    if (Rewrite.overwriteChangedFiles()) {
      llvm::errs() << "nodecpp-instrument failed to apply suggested fixes.\n";
    } else {
      llvm::errs() << "nodecpp-instrument applied suggested fixes.\n";
    }
  }
}



class DezombifyConsumer : public ASTConsumer {
private:
//  CompilerInstance &CI;

public:
//    explicit DezombifyConsumer(CompilerInstance &CI) :CI(CI) {}

    void HandleTranslationUnit(ASTContext &Context) override {
      DzHelper Data;
      Dezombify1ASTVisitor Visitor1(Context, Data);
      DezombiefyRelaxASTVisitor VisitorRelax(Context, Data);
      Dezombify2ASTVisitor Visitor2(Context, Data);

      Visitor1.TraverseDecl(Context.getTranslationUnitDecl());
      VisitorRelax.TraverseDecl(Context.getTranslationUnitDecl());
      Visitor2.TraverseDecl(Context.getTranslationUnitDecl());

      overwriteChangedFiles(Context, Visitor2.getReplacements());
    }
};

class UnwrapperConsumer : public ASTConsumer {
private:
//  CompilerInstance &CI;

public:
//    explicit DezombifyConsumer(CompilerInstance &CI) :CI(CI) {}

    void HandleTranslationUnit(ASTContext &Context) override {
      Deunsequence2ASTVisitor Visitor1(Context);

      Visitor1.TraverseDecl(Context.getTranslationUnitDecl());

      overwriteChangedFiles(Context, Visitor1.getReplacements());
    }
};


} //namespace nodecpp

int main(int argc, const char **argv) {

    //    InitializeAllTargets();
    // LLVMInitializeX86TargetInfo();
    // LLVMInitializeX86TargetMC();
    // LLVMInitializeX86AsmParser();
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();

  CommonOptionsParser optionsParser(argc, argv, NodecppInstrumentCategory, cl::ZeroOrMore);

  ClangTool Tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

  class DezombiefyAction : public ASTFrontendAction {
  protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                  StringRef InFile) override {
      return llvm::make_unique<nodecpp::UnwrapperConsumer>();
    }
  };

  class DezombiefyActionFactory : public FrontendActionFactory {
  public:
    FrontendAction *create() override {
      std::unique_ptr<FrontendAction> WrappedAction(new DezombiefyAction());
      return new nodecpp::ExpandRecompileAction(std::move(WrappedAction), OutputFilename);
    }
  };

  DezombiefyActionFactory Factory;

//  auto FrontendFactory = newFrontendActionFactory<nodecpp::ExpandRecompileAction>();

  Tool.run(&Factory);
}
