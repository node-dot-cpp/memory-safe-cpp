/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
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
static cl::OptionCategory myToolCategory("safe-memory-odr options");

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


struct Element {
    clang::SourceLocation Loc;
    std::string MangledName;
    unsigned OdrHash;
};

struct MappingData {

    std::set<const Element*> AllElements;

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

    // for(auto it = allTypes.begin(); it != allTypes.end(); ++it) {
    //     auto name = getQnameForSystemSafeDb(*it);
    //     allTypeNames.insert(name);
    //     allNames.insert(name);
    // }    

    // for(auto it = allFuncs.begin(); it != allFuncs.end(); ++it) {
    //     auto name = getQnameForSystemSafeDb(*it);
    //     allFuncNames.insert(name);
    //     allNames.insert(name);
    // }    

}

void SerializeData(FILE* file, const MappingData& md) {

    fprintf(file, "[\n{\n");

    // fprintf(file, "  \"names\" : [\n");
    // for(auto it = md.allNames.begin(); it != md.allNames.end(); ++it) {
    //     fprintf(file, "    \"%s\",\n", it->c_str());
    // }    

    // fprintf(file, "  \"types\" : [\n");
    // for(auto it = md.allTypeNames.begin(); it != md.allTypeNames.end(); ++it) {
    //     fprintf(file, "    \"%s\",\n", it->c_str());
    // }    

    // fprintf(file, "  ],\n  \"functions\" : [\n");
    // for(auto it = md.allFuncNames.begin(); it != md.allFuncNames.end(); ++it) {
    //     fprintf(file, "    \"%s\",\n", it->c_str());
    // }    

    fprintf(file, "  ]\n}\n]\n");
}


class CheckNamedDeclVisitor
    : public RecursiveASTVisitor<CheckNamedDeclVisitor> {
private:
    ASTContext *context;
    MappingData& md;

public:
    explicit CheckNamedDeclVisitor(ASTContext *context, MappingData& md)
        : context(context), md(md) {}

    bool VisitCXXRecordDecl(CXXRecordDecl *D) {

        if(D == D->getCanonicalDecl()) {

        }

        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *D) {

        if(D == D->getCanonicalDecl()) {

        }

        return true;
    }

private:


    static bool beginsWith(const string& name, const string& prefix) {
        return name.substr(0, prefix.size()) == prefix;
    }
};

class CheckNamedDeclConsumer : public ASTConsumer {
private:
    MappingData md;

    CheckNamedDeclVisitor visitor1;

    FILE* os;
public:
    explicit CheckNamedDeclConsumer(ASTContext *context, FILE* os)
        : visitor1(context, md), os(os)
    {
        printf("ASTContext: %p\n", context);
    }

    virtual void HandleTranslationUnit(ASTContext &context) {

        auto tu = context.getTranslationUnitDecl();
        printf("ASTContext: %p, Tu: %p\n", &context, tu);
        visitor1.TraverseDecl(context.getTranslationUnitDecl());
        // md.GenerateNames();
        // SerializeData(os, md);
    }
};

class CheckNamedDeclAction : public ASTFrontendAction {
private:
    FILE* os;
public:
    CheckNamedDeclAction(FILE* os) :os(os) {}
    virtual unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler, StringRef inFile) {
        return unique_ptr<ASTConsumer>(new CheckNamedDeclConsumer(&compiler.getASTContext(), os));
    }
};

class CheckNamedDeclActionFactory : public FrontendActionFactory {
private:
    FILE* os;
public:
    CheckNamedDeclActionFactory(FILE* os) :os(os) {}
    FrontendAction *create() override { return new CheckNamedDeclAction(os); }
};

int main(int argc, const char **argv) {

    // llvm::InitializeAllTargets();
    // llvm::InitializeAllTargetInfos();
    // llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();

    CommonOptionsParser optionsParser(argc, argv, myToolCategory);

    ClangTool Tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-DSAFEMEMORY_CHECKER_EXTENSIONS",
            ArgumentInsertPosition::BEGIN));
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-fsyntax-only",
            ArgumentInsertPosition::BEGIN));

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

    CheckNamedDeclActionFactory Factory(f.get());
    return Tool.run(&Factory);
}
