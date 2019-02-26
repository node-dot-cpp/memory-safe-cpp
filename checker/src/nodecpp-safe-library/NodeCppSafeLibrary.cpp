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

#include "raiistdiofile.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static cl::OptionCategory myToolCategory("nodecpp-safe-library options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");

//static cl::opt<bool> Help("h", cl::desc("Alias for -help"), cl::Hidden);

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"), cl::cat(myToolCategory));

static cl::opt<bool>
Dump("dump", cl::desc("Generate an idl tree and dump it.\n"),
    cl::cat(myToolCategory));


struct MappingData {
    std::set<const CXXRecordDecl*> allTypes;
    std::set<const FunctionDecl*> allFuncs;
};


void SerializeData(FILE* file, const MappingData& md) {

    fprintf(file, "[\n{\n");

    fprintf(file, "    \"functions\" : [");
    for(auto it = md.allFuncs.begin(); it != md.allFuncs.end(); ++it) {
        auto name = (*it)->getQualifiedNameAsString();
        fprintf(file, "\"%s\", ", name.c_str());
    }    

    fprintf(file, "],\n    \"types\" : [");
    for(auto it = md.allTypes.begin(); it != md.allTypes.end(); ++it) {
        auto name = (*it)->getQualifiedNameAsString();
        fprintf(file, "\"%s\", ", name.c_str());
    }    
    fprintf(file, "]\n}\n]\n");
}


class FindNamedClassVisitor
    : public RecursiveASTVisitor<FindNamedClassVisitor> {
private:
    ASTContext *context;
    MappingData& md;

public:
    explicit FindNamedClassVisitor(ASTContext *context, MappingData& md)
        : context(context), md(md) {}

    bool VisitCXXRecordDecl(CXXRecordDecl *declaration) {

        auto canon = declaration->getCanonicalDecl();

        md.allTypes.insert(canon);

        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *declaration) {

        auto canon = declaration->getCanonicalDecl();

        md.allFuncs.insert(canon);

        return true;
    }

private:


    static bool beginsWith(const string& name, const string& prefix) {
        return name.substr(0, prefix.size()) == prefix;
    }
};

class FindNamedClassConsumer : public ASTConsumer {
private:
    MappingData md;

    FindNamedClassVisitor visitor1;

    FILE* os;
public:
    explicit FindNamedClassConsumer(ASTContext *context, FILE* os)
        : visitor1(context, md), os(os)
    {}

    virtual void HandleTranslationUnit(ASTContext &context) {
        visitor1.TraverseDecl(context.getTranslationUnitDecl());

        // while(visitor2.doMapping()) {
        //     errs() << "-----------\n";
        //     visitor2.TraverseDecl(context.getTranslationUnitDecl());
        // }

        SerializeData(os, md);
    }
};

class FindNamedClassAction : public ASTFrontendAction {
private:
    FILE* os;
public:
    FindNamedClassAction(FILE* os) :os(os) {}
    virtual unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler, StringRef inFile) {
        return unique_ptr<ASTConsumer>(new FindNamedClassConsumer(&compiler.getASTContext(), os));
    }
};

class FindNamedClassActionFactory : public FrontendActionFactory {
private:
    FILE* os;
public:
    FindNamedClassActionFactory(FILE* os) :os(os) {}
    FrontendAction *create() override { return new FindNamedClassAction(os); }
};

int main(int argc, const char **argv) {

    //    InitializeAllTargets();
    // LLVMInitializeX86TargetInfo();
    // LLVMInitializeX86TargetMC();
    // LLVMInitializeX86AsmParser();
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();

    CommonOptionsParser optionsParser(argc, argv, myToolCategory);

    ClangTool tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

    StringRef FileName("dummy");
    auto PathList = optionsParser.getSourcePathList();
    if (!PathList.empty()) {
        FileName = PathList.front();
    }

    SmallString<256> FilePath(FileName);
//   if (std::error_code EC = llvm::sys::fs::make_absolute(FilePath)) {
//     llvm::errs() << "Can't make absolute path from " << FileName << ": "
//                  << EC.message() << "\n";
//     return 1;
//   }

    SmallString<1024> AbsolutePath(clang::tooling::getAbsolutePath(FilePath));
    StringRef Directory = llvm::sys::path::parent_path(AbsolutePath);



//    vector<string> extraArgs = { "-DHAREIDL_USE_CXX11_ATTRIBUTE" };

    // tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(extraArgs,
    //     ArgumentInsertPosition::BEGIN));

    const char* fname = !OutputFilename.empty() ? OutputFilename.c_str() : "safe_library.json";

    SmallString<1024> JSONDatabasePath(Directory);
    llvm::sys::path::append(JSONDatabasePath, fname);

    RaiiStdioFile f(fopen(JSONDatabasePath.c_str(), "wb"));
    if (!OutputFilename.empty() && !f.get()) {
        errs() << "Failed to open output file '" << OutputFilename.c_str() << "'\n";
        return 1;
    }

    return tool.run(new FindNamedClassActionFactory(f.get()));
}
