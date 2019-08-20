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
#include "InclusionRewriter.h"
#include "DezombiefyRelaxASTVisitor.h"
#include "DeunsequenceASTVisitor.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace llvm::sys::path;
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

// mb: until I figure out how to handle templates, always fix everything
// static cl::opt<bool>
// FixAllUnsequenced("fix-all-unsequenced", cl::desc("Rewrite all unsequenced expressions.\n"),
//     cl::cat(NodecppInstrumentCategory));

namespace nodecpp {


string rewriteFilename(StringRef Filename, const string& NewSuffix) {
  SmallString<128> Path(Filename);
  replace_extension(Path, NewSuffix + extension(Path));
  return Path.str();
}

bool executeAction(FrontendAction* Action, CompilerInstance &CI, const FrontendInputFile &Input) {

  if (!Action->BeginSourceFile(CI, Input))
    return false;
    
  Action->Execute();
  Action->EndSourceFile();

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

  return true;
}

class DezombifyConsumer : public ASTConsumer {
  
  CompilerInstance &CI;

public:
  explicit DezombifyConsumer(CompilerInstance &CI) :CI(CI) {}

  void HandleTranslationUnit(ASTContext &Context) override {

      Dezombify1ASTVisitor Visitor1(Context);
      Dezombify2ASTVisitor Visitor2(Context);
//      Context.getTranslationUnitDecl()->dumpColor();
      Visitor1.TraverseDecl(Context.getTranslationUnitDecl());
      dezombiefyRelax(Context);
      Visitor2.TraverseDecl(Context.getTranslationUnitDecl());

      auto &Reps = Visitor2.finishReplacements(CI.getDiagnostics(), CI.getLangOpts());
      overwriteChangedFiles(Context, Reps, "nodecpp-dezombiefy");
    }
};

class SequenceConsumer : public ASTConsumer {
public:
    void HandleTranslationUnit(ASTContext &Context) override {
      dezombiefySequenceCheckAndFix(Context, false);
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

      overwriteChangedFiles(Context, Visitor1.getReplacements(), "nodecpp-unwrapper");
    }
};
class DezombiefyAction : public ASTFrontendAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                StringRef InFile) override {
    return llvm::make_unique<nodecpp::DezombifyConsumer>(CI);
  }
};

class SequenceAction : public ASTFrontendAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                StringRef InFile) override {
    return llvm::make_unique<nodecpp::SequenceConsumer>();
  }
};

class UnwrapperAction : public ASTFrontendAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                StringRef InFile) override {
    return llvm::make_unique<nodecpp::UnwrapperConsumer>();
  }
};


class ExpandRecompileAction : public PreprocessorFrontendAction {
  std::string Filename;
public:

  ExpandRecompileAction(const std::string& Filename)
    : Filename(Filename) {}

protected:
  void ExecuteAction() override {}


  bool BeginInvocation(CompilerInstance &CI) override {

    const FrontendOptions &FEOpts = CI.getFrontendOpts();
    auto &File = FEOpts.Inputs[0];
    if(Filename.empty())
      Filename = rewriteFilename(File.getFile(), ".instrument");

    error_code EC;
    unique_ptr<raw_fd_ostream> OutputStream;
    OutputStream.reset(new raw_fd_ostream(Filename, EC, sys::fs::F_None));
    if (EC) {
      CI.getDiagnostics().Report(
        diag::err_fe_unable_to_open_output) << Filename << EC.message();
      return false;
    }

    unique_ptr<FrontendAction> ExpandIncludes(new ExpandUserIncludesAction(OutputStream.get()));
    if(!executeAction(ExpandIncludes.get(),  CI, File))
      return false;

    OutputStream->close();
    OutputStream = nullptr;

    PreprocessorOptions &PPOpts = CI.getPreprocessorOpts();
    PPOpts.RemappedFiles.emplace_back(File.getFile().str(), Filename);
    PPOpts.RemappedFilesKeepOriginalName = false;

    std::unique_ptr<FrontendAction> FixSequence(new UnwrapperAction());
    if(!executeAction(FixSequence.get(), CI, File))
      return false;

    unique_ptr<FrontendAction> Dezombiefy(new DezombiefyAction());
    if(!executeAction(Dezombiefy.get(), CI, File))
      return false;
    

    return true;
  }

};

class DezombiefyActionFactory : public FrontendActionFactory {
public:
  FrontendAction *create() override {
    return new ExpandRecompileAction(OutputFilename);
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


  nodecpp::DezombiefyActionFactory Factory;

//  auto FrontendFactory = newFrontendActionFactory<nodecpp::ExpandRecompileAction>();

  Tool.run(&Factory);
}
