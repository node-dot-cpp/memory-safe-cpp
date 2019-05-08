/*******************************************************************************
  Copyright (C) 2016 OLogN Technologies AG
*******************************************************************************/

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Path.h"

#include "Dezombify1ASTVisitor.h"
#include "Dezombify2ASTVisitor.h"


using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static cl::OptionCategory MyToolCategory("nodecpp-dezombify options");

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

    class DezombifyActionFactory {
    public:
        std::unique_ptr<clang::ASTConsumer> newASTConsumer() {
        return llvm::make_unique<nodecpp::DezombifyConsumer>();
        }
    };

  DezombifyActionFactory Factory;

  auto FrontendFactory = newFrontendActionFactory(&Factory);

  Tool.run(FrontendFactory.get());
}
