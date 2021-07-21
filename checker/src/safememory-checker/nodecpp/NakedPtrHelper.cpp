/*******************************************************************************
  Copyright (C) 2019 OLogN Technologies AG
*******************************************************************************/
//===--- NakedPtrHelper.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NakedPtrHelper.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace nodecpp {
namespace checker {

const char *DiagMsgSrc = "memory-safe-cpp";

DiagHelper NullDiagHelper;

bool isOwningPtrName(const std::string &Name) {
  // mb: 'base' ones are needed for methods at isSystemSafeFunction
  return Name == "safememory::owning_ptr" ||
          Name == "safememory::detail::owning_ptr_impl" ||
          Name == "safememory::detail::owning_ptr_base_impl";
}

bool isSafePtrName(const std::string &Name) {
  // mb: 'base' ones are needed for methods at isSystemSafeFunction
  return isOwningPtrName(Name) ||
    Name == "safememory::soft_ptr" ||
    Name == "safememory::detail::soft_ptr_impl" ||
    Name == "safememory::detail::soft_ptr_base_impl" ||
    Name == "safememory::soft_this_ptr" ||
    Name == "safememory::detail::soft_this_ptr_impl" ||
    Name == "safememory::detail::soft_this_ptr_base_impl" ||
    Name == "safememory::soft_this_ptr2" ||
    Name == "safememory::detail::soft_this_ptr2_impl";
}

bool isAwaitableName(const std::string &Name) {
  //mb: hardcoded name, moving to awaitable attribute, then remove this
  return Name == "nodecpp::awaitable";
}

bool isNullablePtrName(const std::string &Name) {
  // mb: 'base' ones are needed for methods at isSystemSafeFunction
  // TODO remove naked_ptr
  return Name == "safememory::nullable_ptr" ||
         Name == "safememory::detail::nullable_ptr_impl" ||
         Name == "safememory::detail::nullable_ptr_base_impl";
}

bool isSoftPtrCastName(const std::string& Name) {
  return Name == "safememory::soft_ptr_static_cast" ||
          Name == "safememory::soft_ptr_reinterpret_cast" ||
          Name == "safememory::nullable_cast";
}

bool isWaitForAllName(const std::string& Name) {
  return Name == "nodecpp::wait_for_all";
}

bool isNodeBaseName(const std::string& Name) {
  return Name == "NodeBase" || 
    Name == "nodecpp::NodeBase" || 
    Name == "nodecpp::net::NodeBase";
}

// bool isStdHashOrEqualToName(const std::string &Name) {
//   return Name == "std::hash" ||
//           Name == "std::equal_to";
// }

bool isStdMoveOrForward(const std::string &Name) {
  return Name == "std::move" ||
          Name == "std::__1::move" || 
          Name == "std::forward" || 
          Name == "std::__1::forward";
}


bool isSystemLocation(const ClangTidyContext *Context, SourceLocation Loc) {

  auto Sm = Context->getSourceManager();
  auto ELoc = Sm->getExpansionLoc(Loc);

  return (ELoc.isInvalid() || Sm->isInSystemHeader(ELoc));
}

bool isSystemSafeTypeName(const ClangTidyContext *Context,
                      const std::string &Name) {

  //hardcode some names that are really important, and have special rules
  if (isSafePtrName(Name) || isNullablePtrName(Name) || isAwaitableName(Name))
    return false;

  if(Context->getCheckerData().isFromUnsafeNamespace(Name))
    return true;

  auto &Wl = Context->getGlobalOptions().SafeTypes;
  return (Wl.find(Name) != Wl.end());
}


bool isStackOnlyIteratorName(const ClangTidyContext *Context,
                      const std::string &Name) {

  //hardcode some names that are really important, and have special rules
    return Name == "eastl::node_iterator" ||
      Name == "safememory::detail::hashtable_stack_only_iterator" ||
      Name == "safememory::detail::array_stack_only_iterator";
}

bool isSystemSafeFunction(const ClangTidyContext* Context, const std::string& Name) {

  if(Context->getCheckerData().isFromUnsafeNamespace(Name))
    return true;

  auto &Wl = Context->getGlobalOptions().SafeFunctions;
  return (Wl.find(Name) != Wl.end());
}

bool isSystemSafeFunction(const FunctionDecl* Decl, const ClangTidyContext* Context) {

  //TODO remove this from here, move to safe-library.json file
  const CXXMethodDecl* M = dyn_cast<CXXMethodDecl>(Decl);
  if(M && M->getParent()) {
    const CXXRecordDecl* Cl = M->getParent();
    std::string ClassName = getQnameForSystemSafeDb(Cl);
    std::string MethodName = M->getNameAsString();
    if(isSafePtrName(ClassName) || isNullablePtrName(ClassName)) {
      if(MethodName == "operator*" || 
              MethodName == "operator->" ||
              MethodName == "operator=" ||
              MethodName == "operator bool" ||
              MethodName == "get")
              return true;
    }
  }

  std::string Name = getQnameForSystemSafeDb(Decl);
  
  // coroutine automatically injected call
  // can't put it into db, so it must be hardcoded
  if (Name == "__builtin_coro_frame")
    return true;

  return isSystemSafeFunction(Context, Name);
}

std::string getQnameForSystemSafeDb(QualType Qt) {

  if(!Qt.isNull()) {
    Qt = Qt.getCanonicalType();
    auto Dc = Qt->getAsCXXRecordDecl();
    if(Dc)
      return getQnameForSystemSafeDb(Dc);
  }

  return "";
}


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

const CXXRecordDecl *getRecordWithDefinition(const CXXRecordDecl *Dc) {

  if (!Dc)
    return nullptr;

  Dc = Dc->getCanonicalDecl();
  if(Dc->hasDefinition())
    return Dc;

  //mb: this can happend with templates that don't add anything when instantiated    
  if(auto TD = dyn_cast<ClassTemplateSpecializationDecl>(Dc))
    Dc = TD->getSpecializedTemplate()->getTemplatedDecl();

  if(Dc->hasDefinition())
    return Dc;

  return nullptr;
}

bool checkNakedStructRecord(const CXXRecordDecl *Dc,
                            const ClangTidyContext *Context, DiagHelper &Dh) {

  Dc = getRecordWithDefinition(Dc);
  if(!Dc)
    return false;

  //we check explicit and implicit here
  bool HasAttr = Dc->hasAttr<SafeMemoryNakedStructAttr>();

  // bool checkInits = false;
  // std::list<const FieldDecl*> missingInitializers;
  auto F = Dc->fields();
  for (auto It = F.begin(); It != F.end(); ++It) {

    auto Qt = (*It)->getType().getCanonicalType();
    if (isSafeType(Qt, Context))
      continue;

    if (auto Np = isNullablePointerQtype(Qt, Context)) {
      if (Np.isOk())
        continue;

      isNullablePointerQtype(Qt, Context, Dh); // for report
      Dh.diag((*It)->getLocation(), "unsafe type at nullable_ptr declaration");
      return false;
    }

    if (auto Ns = isNakedStructType(Qt, Context)) {
      if (Ns.isOk())
        continue;

      isNakedStructType(Qt, Context, Dh); // for report
      Dh.diag((*It)->getLocation(), "unsafe type at naked_struct declaration");
      return false;
    }

    //if none of the above, then is an error
    isSafeType(Qt, Context, Dh);
    Dh.diag((*It)->getLocation(), "member not allowed at naked struct");
    return false;
  }

  auto B = Dc->bases();
  if (B.begin() != B.end()) {
    Dh.diag(B.begin()->getBaseTypeLoc(),
            "inheritance not allowed at naked struct");
    return false;//don't allow any bases yet
  }

  auto M = Dc->methods();
  for (auto It = M.begin(); It != M.end(); ++It) {

    auto Method = *It;

    if (isa<CXXDestructorDecl>(Method))
      continue;

    if (isa<CXXConstructorDecl>(Method)) {
      // if(!missingInitializers.empty()) {
      //   auto inits = ctr->inits();
      //   std::list<const FieldDecl*> decls(missingInitializers);
      //   for(auto jt = inits.begin();jt != inits.end(); ++jt) {
      //     if((*jt)->isMemberInitializer()) {
      //         decls.remove((*jt)->getMember());
      //     }
      //   }

      //   if(!decls.empty()) {
      //     if(check)
      //       check->diag(method->getLocation(), "constructor of naked struct has uninitialized raw pointers");
      //     return false;
      //   }
      // }
      continue;
    }

    if (Method->isConst())
      continue;

    if (Method->isMoveAssignmentOperator() ||
        Method->isCopyAssignmentOperator()) {
      if (Method->isDeleted())
        continue;

      if (HasAttr && Method->isDefaulted())
        continue;
    }

    Dh.diag(Method->getLocation(), "method not allowed at naked struct");
    return false;
  }

  // finally we are safe!
  return true;
}

KindCheck isNakedStructType(QualType Qt, const ClangTidyContext *Context,
                            DiagHelper &Dh) {

  assert(Qt.isCanonical());

  const CXXRecordDecl *Dc = Qt->getAsCXXRecordDecl();

  Dc = getRecordWithDefinition(Dc);
  if (Dc && Dc->hasAttr<SafeMemoryNakedStructAttr>())
    return KindCheck(true, checkNakedStructRecord(Dc, Context));

  //t->dump();
  return KindCheck(false, false);
}

FunctionKind getFunctionKind(QualType Qt) {

  if (auto Ts = Qt->getAs<TemplateSpecializationType>()) {

    // auto Td = Ts->getTemplateName().getAsTemplateDecl();
    // if (Td) {
    //   if (getQnameForSystemSafeDb(Td) == "nodecpp::function_owned_arg0")
    //     return FunctionKind::OwnedArg0;
    // }
    if(Ts->isSugared())
      return getFunctionKind(Ts->desugar());
    else
      return FunctionKind::None;
  } else if (auto Rc = Qt->getAs<RecordType>()) {
    auto Dc = Rc->getDecl();
    assert(Dc);

    if (Dc->isLambda())
      return FunctionKind::Lambda;

    auto Name = getQnameForSystemSafeDb(Dc);
    if (Name == "std::function" || Name == "std::__1::function")
      return FunctionKind::StdFunction;
    
    return FunctionKind::None;
  }

  //qt.dump();
  return FunctionKind::None;
}

bool isStdFunctionType(QualType Qt) {

  return getFunctionKind(Qt) == FunctionKind::StdFunction;
}

bool isLambdaType(QualType Qt) {

  return getFunctionKind(Qt) == FunctionKind::Lambda;
}

bool isRawPointerType(QualType Qt) {
  assert(Qt.isCanonical());

  return Qt->isPointerType();
}

bool isNullPtrValue(ASTContext *Context, const Expr *Ex) {
    return Ex->isNullPointerConstant(*Context, 
      Expr::NullPointerConstantValueDependence::NPC_NeverValueDependent);
}

bool isStringLiteralType(QualType Qt) {
  
  assert(Qt.isCanonical());

  auto R = Qt->getAsCXXRecordDecl();
  if(!R)
    return false;

  auto Name = getQnameForSystemSafeDb(R);
  return Name == "nodecpp::string_literal" || Name == "safememory::basic_string_literal";
}

bool isBasicStringType(QualType Qt) {

  assert(Qt.isCanonical());

  auto R = Qt->getAsCXXRecordDecl();
  if(!R)
    return false;

  auto Name = getQnameForSystemSafeDb(R);
  return Name == "safememory::basic_string" || Name == "safememory::basic_string_safe";
}

bool isCharPointerOrArrayType(QualType Qt) {
  assert(Qt.isCanonical());

  if(Qt->isPointerType() || Qt->isArrayType()) {
    auto Qt2 = Qt->getPointeeOrArrayElementType();
    if(Qt2->isAnyCharacterType())
      return true;
  }
  return false;
}

llvm::Optional<QualType> getPointeeType(QualType Qt) {

  assert(Qt.isCanonical());

  if (Qt->isPointerType())
    return llvm::Optional<QualType>(Qt->getPointeeType().getCanonicalType());

  return getTemplateArgType(Qt, 0);
}

llvm::Optional<QualType> getTemplateArgType(QualType Qt, size_t i) {

  assert(Qt.isCanonical());

  auto Dc = dyn_cast_or_null<ClassTemplateSpecializationDecl>(Qt->getAsCXXRecordDecl());

  if(!Dc)
    return llvm::Optional<QualType>();

  auto &Args = Dc->getTemplateArgs();

  assert(Args.size() > i);

  auto &Argi = Args.get(i);

  assert(Argi.getKind() == TemplateArgument::Type);

  return llvm::Optional<QualType>(Argi.getAsType().getCanonicalType());
}



KindCheck isNullablePointerQtype(QualType Qt, const ClangTidyContext *Context,
                             DiagHelper &Dh) {

  assert(Qt.isCanonical());
  // auto Dc = getTemplatePtrDecl(Qt);
  // if (!Dc)
  //   return KindCheck(false, false);

  std::string Name = getQnameForSystemSafeDb(Qt);
  if (isNullablePtrName(Name)) {
    auto Pointee = getPointeeType(Qt);
    return KindCheck(true, Pointee ? isSafeType(*Pointee, Context, Dh) : false);
  }

  return KindCheck(false, false);
}

SourceLocation getLocationForTemplateArg(const CXXRecordDecl *Rd, unsigned I) {

  auto Dc = dyn_cast_or_null<ClassTemplateSpecializationDecl>(Rd);
  if(Dc) {
    auto Tr = Dc->getTypeAsWritten();
    if(Tr) {
      auto Tl = Tr->getTypeLoc().getAs<clang::TemplateSpecializationTypeLoc>();
      if(Tl) {
        return Tl.getArgLoc(I).getLocation();
      }
    }
  }
  return SourceLocation();
}

bool templateArgIsSafe(QualType Qt, size_t i, const ClangTidyContext* Context, DiagHelper& Dh) {

  auto ArgI = getTemplateArgType(Qt, i);
  if(!ArgI) {
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + "' is not safe");
    return false;
  }

  if(!isSafeType(*ArgI, Context, Dh)) {
    std::string Name = ArgI->getAsString();
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + ", '" + Name + "' is not safe");
    return false;
  }

  return true;
}


bool templateArgIsSafeAndDeepConst(QualType Qt, size_t i, const ClangTidyContext* Context, DiagHelper& Dh) {

  //mb: while [[deep_const]] already implies type safety, is better to check and report
  // for safety first, as this makes the error message easier to understand.
  auto ArgI = getTemplateArgType(Qt, i);
  if(!ArgI) {
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + "' is not safe");
    return false;
  }

  if(!isSafeType(*ArgI, Context, Dh)) {
    std::string Name = ArgI->getAsString();
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + ", '" + Name + "' is not safe");
    return false;
  }

  auto Kc = isDeepConstType(*ArgI, Context, Dh);
  if(!Kc ||!Kc.isOk()) {
    std::string Name = ArgI->getAsString();
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + ", '" + Name + "' is not [[deep_const]]");
    return false;
  }

  return true;
}

bool templateArgIsDeepConstAndNoSideEffectCallOp(QualType Qt, size_t i, const ClangTidyContext* Context, DiagHelper& Dh) {

  //mb: while [[deep_const]] already implies type safety, is better to check and report
  // for safety first, as this makes the error message easier to understand.
  auto ArgI = getTemplateArgType(Qt, i);
  if(!ArgI) {
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + "' is not safe");
    return false;
  }

  if(!isSafeType(*ArgI, Context, Dh)) {
    std::string Name = ArgI->getAsString();
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + ", '" + Name + "' is not safe");
    return false;
  }

  auto Kc = isDeepConstType(*ArgI, Context, Dh);
  if(!Kc ||!Kc.isOk()) {
    std::string Name = ArgI->getAsString();
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + ", '" + Name + "' is not [[deep_const]]");
    return false;
  }

  //mb: hack for std::hash and std::equal_to
  // if(isImplicitDeepConstStdHashOrEqualTo(ArgI, Context, Dh)) {
  //   return true;
  // }

  const CXXRecordDecl* Rd = (*ArgI)->getAsCXXRecordDecl();
  if(Rd)
    Rd = getRecordWithDefinition(Rd);

  if(!Rd) {
    std::string Name = ArgI->getAsString();
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + ", '" + Name + "' must have a [[no_side_effect]] operator()");
    return false;
  }


  bool Found = false;
  auto M = Rd->methods();
  for (auto It = M.begin(); It != M.end(); ++It) {

    auto Method = *It;
    if(Method->isOverloadedOperator()) {
      auto Op = Method->getOverloadedOperator();
      if(Op == OO_Call) {
        
        auto C = const_cast<ClangTidyContext*>(Context);
        if(!C->getCheckHelper()->isNoSideEffect(Method)) {
          std::string Name = ArgI->getAsString();
          Dh.diag(Method->getLocation(), "template parameter " + std::to_string(i) + ", '" + Name + "' must have a [[no_side_effect]] operator()");
          return false;
        }
        else
          Found = true;
     }
    } 
  }
  if(!Found) {
    std::string Name = ArgI->getAsString();
    Dh.diag(SourceLocation(), "template parameter " + std::to_string(i) + ", '" + Name + "' must have a [[no_side_effect]] operator()");
    return false;
  }
  return true;
}

bool allTemplateArgsAreDeepConst(const CXXRecordDecl *Rd, const ClangTidyContext* Context, DiagHelper& Dh) {

//  assert(Rd.isCanonical());

  auto Dc = dyn_cast_or_null<ClassTemplateSpecializationDecl>(Rd);
  if(!Dc)
    return false;



  auto &Args = Dc->getTemplateArgs();
  for(unsigned I = 0; I < Args.size(); ++I) {
    auto &Argi = Args.get(I);
    if(Argi.getKind() == TemplateArgument::Type) {
      QualType Qt = Argi.getAsType().getCanonicalType();
      if(!isSafeType(Qt, Context, Dh)) {
        std::string Name = Qt.getAsString();
        Dh.diag(SourceLocation(), "template parameter " + std::to_string(I) + ", '" + Name + "' is not safe");
        return false;
      }
      auto Kc = isDeepConstType(Qt, Context, Dh);
      if(!Kc || !Kc.isOk()) {
        std::string Name = Qt.getAsString();
        Dh.diag(SourceLocation(), "template parameter " + std::to_string(I) + ", '" + Name + "' is not [[deep_const]]");
        return false;
      }
    }
  }

  return true;
}


KindCheck isSafeVectorType(QualType Qt, const ClangTidyContext* Context,
                             DiagHelper &Dh) {
  
  assert(Qt.isCanonical());
  // auto Dc = getTemplatePtrDecl(Qt);
  // if (!Dc)
  //   return KindCheck(false, false);

  std::string Name = getQnameForSystemSafeDb(Qt);
  if (Name == "safememory::vector" || Name == "safememory::vector_safe") {
    return KindCheck(true, templateArgIsSafe(Qt, 0, Context, Dh));
  }

  return KindCheck(false, false);

}

KindCheck isSafeHashMapType(QualType Qt, const ClangTidyContext* Context,
                             DiagHelper &Dh) {

  assert(Qt.isCanonical());
  // auto Dc = getTemplatePtrDecl(Qt);
  // if (!Dc)
  //   return KindCheck(false, false);

  std::string Name = getQnameForSystemSafeDb(Qt);
  if (Name == "safememory::unordered_map" || Name == "safememory::unordered_map_safe" ||
    Name == "safememory::unordered_multimap" || Name == "safememory::unordered_multimap_safe") {
    // mb: hashmap Key,Hash, and Equal must be deep_const
    // value only needs to be safe

    if(!templateArgIsSafeAndDeepConst(Qt, 0, Context, Dh))
      return KindCheck(true, false);
    if(!templateArgIsSafe(Qt, 1, Context, Dh))
      return KindCheck(true, false);
    if(!templateArgIsDeepConstAndNoSideEffectCallOp(Qt, 2, Context, Dh))
      return KindCheck(true, false);
    if(!templateArgIsDeepConstAndNoSideEffectCallOp(Qt, 3, Context, Dh))
      return KindCheck(true, false);
    return KindCheck(true, true);
  }

  return KindCheck(false, false);

}


bool isDeepConstOwningPtrType(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh) {

  assert(Qt.isCanonical());

  std::string Name = getQnameForSystemSafeDb(Qt);
  if(!isOwningPtrName(Name))
    return false;

  auto Arg = getTemplateArgType(Qt, 0);
  if(!Arg)
    return false;

  auto Kc = isDeepConstType(*Arg, Context, Dh);
  if(!Kc)
    return false;
  else if(!Kc.isOk()) {
    // SourceLocation Sl = getLocationForTemplateArg(Qt->getAsCXXRecordDecl(), 0);
    // Dh.diag(Sl, "owned type is not [[deep_const]]");
    return false;
  }

  if(!Arg->isConstQualified()) {
    // SourceLocation Sl = getLocationForTemplateArg(Qt->getAsCXXRecordDecl(), 0);
    // Dh.diag(Sl, "owned type must be 'const' for owning_ptr to meet deep_const requiremenst");
    return false;
  }

  return true;
}

// bool isImplicitDeepConstStdHashOrEqualTo(QualType Qt, const ClangTidyContext* Context, DiagHelper& Dh) {

//   assert(Qt.isCanonical());

//   std::string Name = getQnameForSystemSafeDb(Qt);
//   if(!isStdHashOrEqualToName(Name))
//     return false;

//   QualType Arg = getTemplateArgType(Qt, 0);
//   auto Kc = isDeepConstType(Arg, Context, Dh);
//   if(!Kc)
//     return false;
//   else if(!Kc.isOk()) {
//     // SourceLocation Sl = getLocationForTemplateArg(Qt->getAsCXXRecordDecl(), 0);
//     // Dh.diag(Sl, "owned type is not [[deep_const]]");
//     return false;
//   }

//   return true;
// }

bool isSafePtrType(QualType Qt) {

  assert(Qt.isCanonical());

  // auto Dc = getTemplatePtrDecl(Qt);
  // if (!Dc)
  //   return false;

  std::string Name = getQnameForSystemSafeDb(Qt);
  return isSafePtrName(Name);
}

bool isAwaitableType(QualType Qt) {

  Qt = Qt.getCanonicalType();

  auto Dc = Qt->getAsCXXRecordDecl();
  if (!Dc)
    return false;

  std::string Name = getQnameForSystemSafeDb(Dc);
  if(isAwaitableName(Name))
    return true;
  else
    return Dc->hasAttr<SafeMemoryAwaitableAttr>();
}

bool isNodecppErrorType(QualType Qt) {

  Qt = Qt.getCanonicalType();

  auto Dc = Qt->getAsCXXRecordDecl();
  if (!Dc)
    return false;

  std::string Name = getQnameForSystemSafeDb(Dc);
  
  return Name == "nodecpp::error";
}


struct SystemLocRiia {
  TypeChecker& Tc;
  bool wasFalse = false;

  SystemLocRiia(TypeChecker& Tc, bool newValue) :
    Tc(Tc) {
      if(newValue) {
        bool oldValue = Tc.swapSystemLoc(newValue);
        if(oldValue == false)
          wasFalse = true;
      }
    }

  ~SystemLocRiia() {
    if(wasFalse)
      Tc.swapSystemLoc(false);
  }

  bool getWasFalse() const { return wasFalse; }
};


bool TypeChecker::isSafeRecord(const CXXRecordDecl *Dc) {

  Dc = getRecordWithDefinition(Dc);
  if (!Dc)
    return false;

  TypeRecursionRiia recursionRiia{*this, Dc};
  if(recursionRiia.wasAlreadyHere()) {
    //already checking this type (got recursive)
    return true;
  }

  // if database says is a safe name, then is safe
  std::string Name = getQnameForSystemSafeDb(Dc);
  if (isSystemSafeTypeName(Context, Name))
    return true;

  bool sysLoc = false;

  if (!isSystemLoc && isSystemLocation(Context, Dc->getLocation())) {
    sysLoc = true;
  }

  SystemLocRiia riia(*this, sysLoc);

  if (Dc->isUnion()) {
    return checkUnion(Dc, Dh);
  }

  auto F = Dc->fields();
  for (auto It = F.begin(); It != F.end(); ++It) {
    auto Ft = (*It)->getType().getCanonicalType();
    if (!isSafeType(Ft)) {
      if(!isSystemLoc || riia.getWasFalse()) {
        std::string Msg =
            "member '" + std::string((*It)->getName()) + "' is not safe";
        Dh.diag((*It)->getLocation(), Msg);
      }
      return false;
    }
  }

  auto B = Dc->bases();
  for (auto It = B.begin(); It != B.end(); ++It) {

    auto Bt = It->getType().getCanonicalType();
    if (!isSafeType(Bt)) {
      if(!isSystemLoc || riia.getWasFalse()) {
        Dh.diag((*It).getBaseTypeLoc(), "base class is not safe");
      }
      return false;
    }
  }

  // finally we are safe!
  return true;
}

bool TypeChecker::isSafeType(const QualType& Qt) {

  assert(Qt.isCanonical());

  if (isStackOnlyQtype(Qt)) {
    return false;
  // } else if (Qt->isReferenceType()) {
  //   return false;
  // } else if (Qt->isPointerType()) {
  //   return false;
  } else if (Qt->isBuiltinType()) {
    return true;
  } else if (Qt->isEnumeralType()) {
    return true;
  } else if (isAwaitableType(Qt)) { //explicit black-list
    return false;
  // } else if (isNullablePointerQtype(Qt, Context)) { //explicit black-list
  //   return false;
  } else if(auto Kc = isSafeVectorType(Qt, Context, Dh)) {
    return Kc.isOk();
  } else if(auto Kc = isSafeHashMapType(Qt, Context, Dh)) {
    return Kc.isOk();
  } else if (isSafePtrType(Qt)) {
    auto Pointee = getPointeeType(Qt);
    return Pointee ? isSafeType(*Pointee) : false;
  } else if (auto Rd = Qt->getAsCXXRecordDecl()) {
    return isSafeRecord(Rd);
  } else if (Qt->isTemplateTypeParmType()) {
    // we will take care at instantiation
    return true;
  } else {
    //t->dump();
    return false;
  }
}


bool TypeChecker::isStackOnlyIteratorRecord(const CXXRecordDecl *Dc) {

  Dc = getRecordWithDefinition(Dc);
  if (!Dc)
    return false;

  // if database says is a safe name, then is safe
  std::string Name = getQnameForSystemSafeDb(Dc);
  return isStackOnlyIteratorName(Context, Name);
}

bool TypeChecker::isStackOnlyIterator(const QualType& Qt2) {

  auto Qt = Qt2.getCanonicalType();
  if(auto Dc = Qt->getAsCXXRecordDecl()) {
    return isStackOnlyIteratorRecord(Dc);
  }
  else
    return false;
}

bool TypeChecker::isStackOnlyQtype(const QualType& Qt2) {

  auto Qt = Qt2.getCanonicalType();

  if(isRawPointerType(Qt))
    return true;
  else if (isNullablePointerQtype(Qt, Context))
    return true;
  else if(Qt->isLValueReferenceType())
    return true;
  else if(isNakedStructType(Qt, Context))
    return true;
  else if(auto Rd = Qt->getAsCXXRecordDecl())
    return isStackOnlyIteratorRecord(Rd);
  else {
    //t->dump();
    return false;
  }
}


bool TypeChecker::isDeterministicRecord(const CXXRecordDecl *Dc) {

  Dc = getRecordWithDefinition(Dc);
  if (!Dc)
    return false;

  if(!alreadyChecking.insert(Dc).second) {
    //already checking this type (got recursive)
    return true;
  }

  // if database says is a safe name, then is safe
  std::string Name = getQnameForSystemSafeDb(Dc);
  if (isSystemSafeTypeName(Context, Name))
    return true;

  bool sysLoc = false;

  if (!isSystemLoc && isSystemLocation(Context, Dc->getLocation())) {
    sysLoc = true;
  }

  SystemLocRiia riia(*this, sysLoc);

  if (Dc->isUnion()) {

    // for unions, one initializer is enought.
    // the rest of the union is zero padded.

    auto F = Dc->fields();
    for (auto It = F.begin(); It != F.end(); ++It) {
      if (It->hasInClassInitializer()) {
        return true;
      }
    }
    return false;
  }

  auto F = Dc->fields();
  for (auto It = F.begin(); It != F.end(); ++It) {
    auto Ft = (*It)->getType().getCanonicalType();
    if (!isDeterministicType(Ft) && !It->hasInClassInitializer()) {
      if(!isSystemLoc || riia.getWasFalse()) {
        std::string Msg =
            "member '" + std::string((*It)->getName()) + "' is not deterministic";
        Dh.diag((*It)->getLocation(), Msg);
      }
      return false;
    }
  }

  auto B = Dc->bases();
  for (auto It = B.begin(); It != B.end(); ++It) {

    auto Bt = It->getType().getCanonicalType();
    if (!isDeterministicType(Bt)) {
      if(!isSystemLoc || riia.getWasFalse()) {
        Dh.diag((*It).getBaseTypeLoc(), "base class is not deterministic");
      }
      return false;
    }
  }

  // finally we are deterministic!
  return true;
}

bool TypeChecker::isDeterministicType(const QualType& Qt) {

  assert(Qt.isCanonical());

  if (Qt->isReferenceType()) {
    return true;
  } else if (Qt->isPointerType()) {
    return false;
  } else if (Qt->isBuiltinType()) {
    return false;
  } else if (Qt->isEnumeralType()) {
    return false;
  } else if (isAwaitableType(Qt)) {//mb: check
    return true;
  } else if (isLambdaType(Qt)) {
    return true;
  } else if (isSafePtrType(Qt)) {
    return true;
  } else if (isNullablePointerQtype(Qt, Context)) {
    return true;
  } else if (auto Rd = Qt->getAsCXXRecordDecl()) {
    return isDeterministicRecord(Rd);
  } else if (Qt->isTemplateTypeParmType()) {
    // we will take care at instantiation
    return true;
  } else {
    //t->dump();
    return false;
  }
}


KindCheck TypeChecker::isDeepConstRecord(const CXXRecordDecl *Dc) {

  Dc = getRecordWithDefinition(Dc);
  if (!Dc)
    return {false, false};

  if(!alreadyChecking.insert(Dc).second) {
    //already checking this type (got recursive)
    return {true, true};
  }




  bool sysLoc = false;

  if (!isSystemLoc && isSystemLocation(Context, Dc->getLocation())) {
    sysLoc = true;
  }

  SystemLocRiia riia(*this, sysLoc);
  bool attr = Dc->hasAttr<SafeMemoryDeepConstAttr>();
  bool attrWhenParams = Dc->hasAttr<SafeMemoryDeepConstWhenParamsAttr>();

  if(isSystemLoc) {
    if(attr)
      return {true, true};
    else if(attrWhenParams) {
      bool params = allTemplateArgsAreDeepConst(Dc, Context, Dh);
      return {params, params};
    }
    else
      return {false, false};
  }
  else {
    if(!attr)
      return {false, false};
   
    // else fall down to analyze
  }

  // below this line, we must return true for 'isKind'

  if (Dc->isUnion()) {
    //not supported yet
    Dh.diag(Dc->getLocation(), "Union not supported yet");
    return {true, false};
  }

  auto F = Dc->fields();
  for (auto It = F.begin(); It != F.end(); ++It) {
    auto Ft = (*It)->getType();
    auto Kc = isDeepConstType(Ft);
    if (!Kc || !Kc.isOk()) {
      if(!isSystemLoc || riia.getWasFalse()) {
        std::string Msg =
            "member '" + std::string((*It)->getName()) + "' is not deep const";
        Dh.diag((*It)->getLocation(), Msg);
      }
      return {true, false};
    }
  }

  auto M = Dc->methods();
  for (auto It = M.begin(); It != M.end(); ++It) {

    auto Method = *It;

    if (isa<CXXDestructorDecl>(Method)) {
      if(!Method->isDefaulted()) {
        Dh.diag((*It)->getLocation(), "desctructor must be 'default' for deep const");
        return {true, false};
      }
    }
    else if (auto Ctor = dyn_cast<CXXConstructorDecl>(Method)) {
      if(Ctor->isCopyOrMoveConstructor() && !Ctor->isDefaulted()) {
        Dh.diag((*It)->getLocation(), "copy/move constructor must be 'default' for deep const");
        return {true, false};
      }
    }
    else if (Method->isMoveAssignmentOperator() ||
        Method->isCopyAssignmentOperator()) {
      if(!Method->isDefaulted()) {
        Dh.diag((*It)->getLocation(), "copy/move assignment operator must be 'default' for deep const");
        return {true, false};
      }
    }
  }

  auto B = Dc->bases();
  for (auto It = B.begin(); It != B.end(); ++It) {

    auto Bt = It->getType();
    auto Kc = isDeepConstType(Bt);
    if (!Kc || !Kc.isOk()) {
      if(!isSystemLoc || riia.getWasFalse()) {
        Dh.diag((*It).getBaseTypeLoc(), "base class is not deep const");
      }
      return {true, false};
    }
  }

  // finally we are deep const!
  return {true, true};
}

KindCheck TypeChecker::isDeepConstType(QualType Qt) {

  Qt = Qt.getCanonicalType();

  if (Qt->isReferenceType()) {
    return {false, false};
  } else if (Qt->isPointerType()) {
    return {false, false};
  } else if (Qt->isBuiltinType()) {
    return {true, true};
  } else if (Qt->isEnumeralType()) {
    return {true, true};
  // } else if (isAwaitableType(Qt)) {//mb: check
  //   return false;
  // } else if (isLambdaType(Qt)) {
  //   return false;
  
  // } else if (isImplicitDeepConstStdHashOrEqualTo(Qt, Context, Dh)) {
  //   return {true, true};
  } else if (isDeepConstOwningPtrType(Qt, Context, Dh)) {
    return {true, true};
  } else if (isSafePtrType(Qt)) {
    return {false, false};
  } else if (isNullablePointerQtype(Qt, Context, Dh)) {
    return {false, false};
  } else if (auto Rd = Qt->getAsCXXRecordDecl()) {
    return isDeepConstRecord(Rd);
  } else if (Qt->isTemplateTypeParmType()) {
    // we will take care at instantiation
    return {true, true};
  } else {
    //t->dump();
    return {false, false};
  }
}

const CXXRecordDecl *isUnionType(QualType Qt) {

  assert(Qt.isCanonical());

  const CXXRecordDecl *Rd = Qt->getAsCXXRecordDecl();
  Rd = getRecordWithDefinition(Rd);
 
  return (Rd && Rd->isUnion()) ? Rd : nullptr;
}

bool checkUnion(const CXXRecordDecl *Dc, DiagHelper &Dh) {

  assert(Dc);
  assert(Dc->hasDefinition());
  assert(Dc->isUnion());

  auto F = Dc->fields();
  for (auto It = F.begin(); It != F.end(); ++It) {

    auto Qt = (*It)->getType().getCanonicalType();
    if (isRawPointerType(Qt)) {

      Dh.diag((*It)->getLocation(),
              "(S1.4) raw pointers inside unions are prohibited");
      return false;
    } else if (!Qt->isBuiltinType() && !Qt->isTemplateTypeParmType()) {

      Dh.diag((*It)->getLocation(),
              "(S1.4) non-primitives inside unions are prohibited");
      return false;
    }
  }

  // finally we are safe!
  return true;
}

bool isOsnPtrRecord(const CXXRecordDecl *Dc) {

  if (!Dc)
    return false;

  std::string Name = getQnameForSystemSafeDb(Dc);

  return isSafePtrName(Name) || isNullablePtrName(Name);
}

const Expr *getBaseIfOsnPtrDerref(const Expr *Ex) {

  if (!Ex)
    return nullptr;

  if (auto OpCall = dyn_cast<CXXOperatorCallExpr>(Ex)) {
    if (auto OpDecl = dyn_cast<CXXMethodDecl>(OpCall->getDirectCallee())) {
      if (isOsnPtrRecord(OpDecl->getParent())) {
        auto Name = OpDecl->getDeclName();
        if (Name.getNameKind() == DeclarationName::CXXOperatorName) {
          auto Op = Name.getCXXOverloadedOperator();
          if (Op == OO_Star || Op == OO_Arrow) {
            assert(OpCall->getNumArgs() > 0);
            return OpCall->getArg(0);
          }
        }
      }
    }
  }
  //TODO check for method get()
  return nullptr;
}

bool isImplicitExpr(const Expr *Ex) {
  return isa<ExprWithCleanups>(Ex) ||
    isa<MaterializeTemporaryExpr>(Ex) ||
    isa<CXXBindTemporaryExpr>(Ex) ||
    isa<ImplicitCastExpr>(Ex) ||
    isa<ParenExpr>(Ex);
}

const Expr *getParentExpr(ASTContext *Context, const Expr *Ex) {

  auto SList = Context->getParents(*Ex);

  auto SIt = SList.begin();

  if (SIt == SList.end())
    return nullptr;

  auto P = SIt->get<Expr>();
  if (!P)
    return nullptr;

  if (isImplicitExpr(P))
    return getParentExpr(Context, P);
  else
    return P;
}

const Expr *ignoreTemporaries(const Expr *Ex) {
  if (auto B = dyn_cast<CXXBindTemporaryExpr>(Ex)) {
    return ignoreTemporaries(B->getSubExpr());
  } else if (auto C = dyn_cast<CXXConstructExpr>(Ex)) {
    return ignoreTemporaries(C->getArg(0));
  } else if (auto T = dyn_cast<MaterializeTemporaryExpr>(Ex)) {
    return ignoreTemporaries(T->getSubExpr());
  } else if (auto I = dyn_cast<ImplicitCastExpr>(Ex)) {
    return ignoreTemporaries(I->getSubExpr());
  } else if (auto Cl = dyn_cast<ExprWithCleanups>(Ex)) {
    return ignoreTemporaries(Cl->getSubExpr());
  } else {
    return Ex;
  }
} // namespace nodecpp

DeclRefExpr *getStdMoveArg(Expr *Ex) {
  
  CXXConstructExpr *Ctor = dyn_cast<CXXConstructExpr>(Ex);

  if(!Ctor)
    return nullptr;
  
  if(Ctor->getNumArgs() != 1)
    return nullptr;

  CallExpr *Call = dyn_cast<CallExpr>(Ctor->getArg(0));
  if(!Call)
    return nullptr;
  
  if(!Call->isCallToStdMove())
    return nullptr;

  if(Call->getNumArgs() != 1)
    return nullptr;

  DeclRefExpr *Dre = dyn_cast<DeclRefExpr>(Call->getArg(0));
  return Dre;
}

bool isFunctionPtr(const Expr *Ex) {

  if (!Ex)
    return false;

  auto E = ignoreTemporaries(Ex);

  if (auto Op = dyn_cast<UnaryOperator>(E)) {
    if (Op->getOpcode() != UnaryOperatorKind::UO_AddrOf)
      return false;

    E = Op->getSubExpr();
  }

  if (auto Ref = dyn_cast<DeclRefExpr>(E)) {
    return isa<FunctionDecl>(Ref->getDecl());
  }

  return false;
}

const Stmt *getParentStmt(ASTContext *Context, const Stmt *St) {

  auto SList = Context->getParents(*St);

  auto SIt = SList.begin();

  if (SIt != SList.end())
    return SIt->get<Stmt>();
  else
    return nullptr;
}

const DeclStmt *getParentDeclStmt(ASTContext *Context, const Decl *Dc) {

  if (!Dc)
    return nullptr;

  auto L = Context->getParents(*Dc);

  if (L.begin() != L.end())
    return L.begin()->get<DeclStmt>();
  else
    return nullptr;
}


const FunctionDecl *getEnclosingFunctionDecl(ASTContext *Context, const Stmt *St) {

  if (!St)
    return nullptr;

  auto L = Context->getParents(*St);

  if (L.begin() != L.end()) {
    if (auto Ch = L.begin()->get<Stmt>())
      return getEnclosingFunctionDecl(Context, Ch);
    else if(auto Ch = L.begin()->get<FunctionDecl>())
      return Ch;
  }

  return nullptr;
}

const CXXRecordDecl* getEnclosingCXXRecordDecl(ASTContext *Context, const Stmt* St) {

  if (!St)
    return nullptr;

  auto L = Context->getParents(*St);

  if (L.begin() != L.end()) {
    if (auto Ch = L.begin()->get<Stmt>())
      return getEnclosingCXXRecordDecl(Context, Ch);
    else if(auto Ch = L.begin()->get<CXXMethodDecl>())
      return Ch->getParent();
  }

  return nullptr;
}

const CXXConstructorDecl* getEnclosingCXXConstructorDeclForInit(ASTContext *Context, const Stmt* St) {

  if (!St)
    return nullptr;

  auto L = Context->getParents(*St);

  // so far this only works for expressions inside initializers of constructors
  if (L.begin() != L.end()) {
    if (auto P = L.begin()->get<Stmt>())
      return getEnclosingCXXConstructorDeclForInit(Context, P);
    else if(auto Ctor = L.begin()->get<CXXConstructorDecl>()) {
      return Ctor;
    }
    else if(auto Init = L.begin()->get<CXXCtorInitializer>()) {
      auto L2 = Context->getParents(*Init);
      if (L2.begin() != L2.end()) {
        if(auto Ctor = L2.begin()->get<CXXConstructorDecl>()) {
          return Ctor;
        }
      }
    }
  }

  return nullptr;
}


bool isParmVarOrCatchVar(ASTContext *Context, const VarDecl *D) {
  
  assert(D);

  if(isa<ParmVarDecl>(D))
    return true;

  auto L = Context->getParents(*D);

  if (L.begin() != L.end())
    return L.begin()->get<CXXCatchStmt>();
  else
    return false;
}

bool isDerivedFromNodeBase(QualType Qt) {

  auto T = Qt.getCanonicalType().getTypePtr();

  if(!T)
    return false;

  auto R = T->getAsCXXRecordDecl();
  if(!R)
    return false;

  std::string Name = getQnameForSystemSafeDb(R);
  if(isNodeBaseName(Name))
    return true;

  if(!R->hasDefinition())
    return false;

  for(auto Each : R->bases()) {
    if(isDerivedFromNodeBase(Each.getType()))
      return true;
  }

  return false;
}

bool isEmptyClass(QualType Qt) {

  const clang::Type* T = Qt.getCanonicalType().getTypePtr();

  if(!T)
    return false;

  const CXXRecordDecl* Rd = T->getAsCXXRecordDecl();
  Rd = getRecordWithDefinition(Rd);
  if(!Rd)
    return false;

  return Rd->isEmpty();

  // //mb: remove this
  // if(!R->hasDefinition())
  //   return false;

  // if(!R->field_empty())
  //   return false;

  // for(auto Each : R->bases()) {
  //   if(!isEmptyClass(Each.getType()))
  //     return false;
  // }

  // return true;
}

bool NakedPtrScopeChecker::canArgumentGenerateOutput(QualType Out,
                                                     QualType Arg) {

  // mb: need to rethink this
  return true;
  // out.dump();
  // arg.dump();
}

bool NakedPtrScopeChecker::checkStack2StackAssignment(const Decl *FromDecl) {

  if(!AstContext || !OutScopeDecl) {
    Check->diag(FromDecl->getLocation(),
                "Internal checker error, please report", DiagnosticIDs::Error);
    return false;
  }

  auto FromStmt = getParentDeclStmt(AstContext, FromDecl);
  if (!FromStmt)
    return false;

  auto ToStmt = getParentDeclStmt(AstContext, OutScopeDecl);
  if (!ToStmt)
    return false;

  auto From = getParentStmt(AstContext, FromStmt);
  if (!From)
    return false;

  auto To = getParentStmt(AstContext, ToStmt);
  while (To) {
    if (To == From)
      return true;

    To = getParentStmt(AstContext, To);
  }

  // we couldn't verify this is ok, assume the worst
  return false;
}

bool NakedPtrScopeChecker::checkDeclRefExpr(const DeclRefExpr *DeclRef) {
  auto FromDecl = DeclRef->getDecl();
  if (!FromDecl) { // shouln't happend here
    return false;
  }

  if (isa<FieldDecl>(FromDecl)) {
    FromDecl->dumpColor();
    assert(false);
  } else if (auto ParamVar = dyn_cast<ParmVarDecl>(FromDecl)) {
    switch (OutScope) {
    case Stack:
    case Param:
      return true;
    case This:
      return ParamVar->hasAttr<SafeMemoryMayExtendAttr>();
    default:
      assert(false);
    }
  } else if (auto Var = dyn_cast<VarDecl>(FromDecl)) {
    if (Var->hasGlobalStorage())
      return true;
    else if (Var->hasAttr<SafeMemoryMayExtendAttr>()) {
      return true;
    } else {
      if (OutScope == Stack) {
        return checkStack2StackAssignment(FromDecl);
      }
      return false;
    }
  }

  return false;
}

bool NakedPtrScopeChecker::checkCallExpr(const CallExpr *Call) {

  if (!Call) { // shouln't happend here
    assert(false);
    return false;
  }

  auto CallDecl = Call->getDirectCallee();
  if (!CallDecl) {
    return false;
  }

  if (isa<CXXMemberCallExpr>(Call)) {
    auto Callee = dyn_cast<MemberExpr>(Call->getCallee());

    if (!Callee) {
      //can this happend?
      return false;
    }

    if (isSafePtrType(Callee->getBase()->getType().getCanonicalType()))
      return true;

    if (!checkExpr(Callee->getBase())) {
      return false;
    }
  }
  // TODO check if callee is not the first argument

  auto Params = CallDecl->parameters();
  auto Ret = CallDecl->getReturnType().getCanonicalType();
  auto Args = Call->arguments();

  auto It = Args.begin();
  auto Jt = Params.begin();

  if (isa<CXXOperatorCallExpr>(Call) && isa<CXXMethodDecl>(CallDecl)) {
    // this is an instance method operator call
    // then first argument is actually the instance
    if (It == Args.end()) {
      assert(false);
      return false;
    }

    if (isSafePtrType((*It)->getType().getCanonicalType()))
      return true;

    if (!checkExpr(*It)) {
      return false;
    }
    ++It;
  }

  while (It != Args.end() && Jt != Params.end()) {
    auto Arg = (*Jt)->getType().getCanonicalType();
    if (canArgumentGenerateOutput(Ret, Arg)) {
      // diag((*it)->getExprLoc(),
      //     "check this argument");

      if (!checkExpr(*It)) {
        return false;
      }
    }
    ++It;
    ++Jt;
  }
  if (It != Args.end() || Jt != Params.end()) {
    //TODO diag
    return false;
  }

  // this is ok!
  return true;
}

bool NakedPtrScopeChecker::checkCXXConstructExpr(
    const CXXConstructExpr *Construct) {

  auto Args = Construct->arguments();

  for (auto It = Args.begin(); It != Args.end(); ++It) {

    if (!checkExpr(*It)) {
      return false;
    }
  }

  // this is ok!
  return true;
}

bool NakedPtrScopeChecker::checkExpr(const Expr *From) {

  if(OutScope == Unknown) {
    return false;
  }

  if (!From) { // shouln't happend here
    assert(false);
    return false;
  }

  From = From->IgnoreParenImpCasts();
  if (isa<CXXThisExpr>(From)) {
    switch (OutScope) {
    case Stack:
    case Param:
    case This:
      return true;
    default:
      assert(false);
      return false;
    }
  } else if (isa<CXXNullPtrLiteralExpr>(From)) {
    return true;
  } else if (isa<IntegerLiteral>(From)) {
    return true;
  } else if (auto DeclRef = dyn_cast<DeclRefExpr>(From)) {
    return checkDeclRefExpr(DeclRef);
  } else if (auto Call = dyn_cast<CallExpr>(From)) {
    // this will check functions, members and both operators
    return checkCallExpr(Call);
  } else if (auto Member = dyn_cast<MemberExpr>(From)) {
    // TODO verify only members and not methods will get in here
    return checkExpr(Member->getBase());
  } else if (auto Op = dyn_cast<UnaryOperator>(From)) {
    return checkExpr(Op->getSubExpr());
  } else if (auto Op = dyn_cast<BinaryOperator>(From)) {
    if (checkExpr(Op->getLHS()))
      return checkExpr(Op->getRHS());
    else    
      return false;
  } else if (auto DefArg = dyn_cast<CXXDefaultArgExpr>(From)) {
    return checkExpr(DefArg->getExpr());
  } else if (auto Op = dyn_cast<ConditionalOperator>(From)) {
    //check both branches
    if (checkExpr(Op->getTrueExpr()))
      return checkExpr(Op->getFalseExpr());
    else    
      return false;
  } else if (auto Cast = dyn_cast<CastExpr>(From)) {
    return checkExpr(Cast->getSubExpr());
  } else if (auto Tmp = dyn_cast<MaterializeTemporaryExpr>(From)) {
    return checkExpr(Tmp->getSubExpr());
  } else if (auto Construct = dyn_cast<CXXConstructExpr>(From)) {
    return checkCXXConstructExpr(Construct);
  } else if (auto B = dyn_cast<CXXBindTemporaryExpr>(From)) {
    return checkExpr(B->getSubExpr());
  }

  //just in case
  From->dumpColor();
  return false;
}

/* static */
std::pair<NakedPtrScopeChecker::OutputScope, const Decl *>
NakedPtrScopeChecker::calculateScope(const Expr *Ex) {

  // Scope is only calculated for the expression acting as lhs
  // all other expressions are checked againt them
  if (!Ex) {
    assert(Ex);
    return std::make_pair(Unknown, nullptr);
  }

  Ex = Ex->IgnoreParenImpCasts();
  if (auto DeclRef = dyn_cast<DeclRefExpr>(Ex)) {
    auto Dc = DeclRef->getDecl();
    if (!Dc) { // shouldn't happend here
      return std::make_pair(Unknown, nullptr);
    }

    if (auto ParmVar = dyn_cast<ParmVarDecl>(Dc)) {

      if (ParmVar->hasAttr<SafeMemoryMayExtendAttr>())
        return std::make_pair(This, nullptr);
      else
        return std::make_pair(Param, nullptr);
    } else if (dyn_cast<FieldDecl>(Dc)) {

      Ex->dumpColor();
      assert(false);
      return std::make_pair(Unknown, nullptr);
    } else if (auto Var = dyn_cast<VarDecl>(Dc)) {
      if (Var->hasGlobalStorage()) // globals can't be changed
        return std::make_pair(Unknown, nullptr);
      else if (Var->hasAttr<SafeMemoryMayExtendAttr>())
        return std::make_pair(This, nullptr);
      else
        return std::make_pair(Stack, Dc);
    }
  } else if (auto Member = dyn_cast<MemberExpr>(Ex)) {
    // TODO verify only members and not methods will get in here
    // TODO also verify is a hard member and not a pointer / unique_ptr
    return calculateScope(Member->getBase());
  } else if (isa<CXXThisExpr>(Ex)) {
    return std::make_pair(This, nullptr);
  }

  llvm::errs() << "NakedPtrScopeChecker::calculateScope > Unknown\n";
  Ex->dumpColor();
  return std::make_pair(Unknown, nullptr);
}

/* static */
NakedPtrScopeChecker
NakedPtrScopeChecker::makeChecker(ClangTidyCheck *Check,
                                  ClangTidyContext *TidyContext,
                                  ASTContext *AstContext, const Expr *ToExpr) {

  auto Sc = NakedPtrScopeChecker::calculateScope(ToExpr);

  return NakedPtrScopeChecker(Check, TidyContext, AstContext, Sc.first,
                              Sc.second);
}

/* static */
NakedPtrScopeChecker
NakedPtrScopeChecker::makeThisScopeChecker(ClangTidyCheck *Check,
                                           ClangTidyContext *TidyContext) {

  return NakedPtrScopeChecker(Check, TidyContext, nullptr, This, nullptr);
}
/* static */
NakedPtrScopeChecker
NakedPtrScopeChecker::makeParamScopeChecker(ClangTidyCheck *Check,
                                            ClangTidyContext *TidyContext) {

  return NakedPtrScopeChecker(Check, TidyContext, nullptr, Param, nullptr);
}

} // namespace checker
} // namespace nodecpp
