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
static cl::OptionCategory MyToolCategory("nodecpp-instrument options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");

//static cl::opt<bool> Help("h", cl::desc("Alias for -help"), cl::Hidden);

// static cl::opt<std::string>
// OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"), cl::cat(myToolCategory));

// static cl::opt<bool>
// Dump("dump", cl::desc("Generate an idl tree and dump it.\n"),
//     cl::cat(myToolCategory));

namespace nodecpp {

class DezombifyConsumer : public ASTConsumer {
private:
//    MappingData md;

//    FindNamedClassVisitor visitor1;

//    FILE* os;
public:
    // explicit DezombifyConsumer(ASTContext *context, FILE* os)
    //     : visitor1(context, md), os(os)
    // {}

    void HandleTranslationUnit(ASTContext &Context) override {
       Dezombify1ASTVisitor Visitor1(Context);
       Dezombify2ASTVisitor Visitor2(Context);

       Visitor1.TraverseDecl(Context.getTranslationUnitDecl());
       Visitor2.TraverseDecl(Context.getTranslationUnitDecl());

       Visitor2.overwriteChangedFiles();
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

    CommonOptionsParser optionsParser(argc, argv, MyToolCategory);

    ClangTool Tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

    // StringRef FileName("dummy");
    // auto PathList = optionsParser.getSourcePathList();
    // if (!PathList.empty()) {
    //     FileName = PathList.front();
    // }

    // SmallString<256> FilePath(FileName);
//   if (std::error_code EC = llvm::sys::fs::make_absolute(FilePath)) {
//     llvm::errs() << "Can't make absolute path from " << FileName << ": "
//                  << EC.message() << "\n";
//     return 1;
//   }

    // SmallString<1024> AbsolutePath(clang::tooling::getAbsolutePath(FilePath));
    // StringRef Directory = llvm::sys::path::parent_path(AbsolutePath);



//    vector<string> extraArgs = { "-DHAREIDL_USE_CXX11_ATTRIBUTE" };

    // tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(extraArgs,
    //     ArgumentInsertPosition::BEGIN));

    // const char* fname = !OutputFilename.empty() ? OutputFilename.c_str() : "safe_library.json";

    // SmallString<1024> JSONDatabasePath(Directory);
    // llvm::sys::path::append(JSONDatabasePath, fname);

    // RaiiStdioFile f(fopen(JSONDatabasePath.c_str(), "wb"));
    // if (!OutputFilename.empty() && !f.get()) {
    //     errs() << "Failed to open output file '" << OutputFilename.c_str() << "'\n";
    //     return 1;
    // }

  class DezombiefyAction : public ASTFrontendAction {
  protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                  StringRef InFile) override {
      return llvm::make_unique<nodecpp::DezombifyConsumer>();
    }
  };

  class DezombiefyActionFactory : public FrontendActionFactory {
  public:
    FrontendAction *create() override {
      std::unique_ptr<FrontendAction> WrappedAction(new DezombiefyAction());
      return new nodecpp::ExpandRecompileAction(std::move(WrappedAction));
    }
  };

  DezombiefyActionFactory Factory;

//  auto FrontendFactory = newFrontendActionFactory<nodecpp::ExpandRecompileAction>();

  Tool.run(&Factory);
}
