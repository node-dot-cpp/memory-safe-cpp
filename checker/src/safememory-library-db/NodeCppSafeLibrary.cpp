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
static cl::OptionCategory myToolCategory("safememory-library-db options");

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

    std::set<std::string> allTypeNames;
    std::set<std::string> allFuncNames;
    std::set<std::string> allNames;

    void GenerateNames();
};

std::string getQnameForSystemSafeDb(const NamedDecl *Decl) {
  
  std::string QualName;
  llvm::raw_string_ostream OS(QualName);

  const DeclContext *Ctx = Decl->getDeclContext();

  using ContextsTy = SmallVector<const DeclContext *, 8>;
  ContextsTy NamedCtxs;

  // Collect named contexts.
  while (Ctx) {
    if (isa<NamedDecl>(Ctx))
      NamedCtxs.push_back(Ctx);
    Ctx = Ctx->getParent();
  }

  for (const DeclContext *DC : llvm::reverse(NamedCtxs)) {
    if (const auto *Spec = dyn_cast<ClassTemplateSpecializationDecl>(DC)) {
      OS << Spec->getNameAsString();
    } else if (const auto *ND = dyn_cast<NamespaceDecl>(DC)) {
      OS <<  ND->getNameAsString();
    } else if (const auto *RD = dyn_cast<RecordDecl>(DC)) {
      OS << RD->getNameAsString();
    } else if (const auto *FD = dyn_cast<FunctionDecl>(DC)) {
      OS << FD->getNameAsString();
    } else if (const auto *ED = dyn_cast<EnumDecl>(DC)) {
      OS << ED->getNameAsString();
    } else {
      OS << cast<NamedDecl>(DC)->getNameAsString();
    }
    OS << "::";
  }

  // if (Decl->getDeclName() || isa<DecompositionDecl>(Decl))
  OS << Decl->getNameAsString();
  // else
  //   return "";
//    OS << "(anonymous)";

  return OS.str();

}


void MappingData::GenerateNames() {
    // We generate all names and put them into set, so they get
    // lexicograph ordered, and make the tool deterministic

    for(auto it = allTypes.begin(); it != allTypes.end(); ++it) {
        auto name = getQnameForSystemSafeDb(*it);
        allTypeNames.insert(name);
        allNames.insert(name);
    }    

    for(auto it = allFuncs.begin(); it != allFuncs.end(); ++it) {
        auto name = getQnameForSystemSafeDb(*it);
        allFuncNames.insert(name);
        allNames.insert(name);
    }
}

void SerializeData(FILE* file, const MappingData& md) {

    fprintf(file, "[\n{\n");

    fprintf(file, "  \"types\" : [\n");
    for(auto it = md.allTypeNames.begin(); it != md.allTypeNames.end(); ++it) {
        fprintf(file, "    \"%s\",\n", it->c_str());
    }

    fprintf(file, "  ],\n  \"functions\" : [\n");
    for(auto it = md.allFuncNames.begin(); it != md.allFuncNames.end(); ++it) {
        fprintf(file, "    \"%s\",\n", it->c_str());
    }

    fprintf(file, "  ]\n}\n]\n");
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
        md.GenerateNames();
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
    std::unique_ptr<FrontendAction> create() override { return std::unique_ptr<FrontendAction>(new FindNamedClassAction(os)); }
};

int main(int argc, const char **argv) {

    // llvm::InitializeAllTargets();
    // llvm::InitializeAllTargetInfos();
    // llvm::InitializeAllTargetMCs();
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
