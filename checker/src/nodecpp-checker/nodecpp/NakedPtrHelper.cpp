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

DiagHelper NullDiagHelper(nullptr);

bool isOwnerPtrName(const std::string &Name) {
  return Name == "nodecpp::safememory::owning_ptr" ||
         Name == "nodecpp::safememory::owning_ptr_impl" ||
         Name == "nodecpp::safememory::owning_ptr_no_checks";
}

bool isOwnerPtrDecl(const NamedDecl *Dc) {
  if (!Dc)
    return false;

  std::string Name = Dc->getQualifiedNameAsString();
  return isOwnerPtrName(Name);
}

bool isSafePtrName(const std::string &Name) {
  return isOwnerPtrName(Name) ||
    Name == "nodecpp::safememory::soft_ptr" ||
    Name == "nodecpp::safememory::soft_ptr_impl" ||
    Name == "nodecpp::safememory::soft_ptr_no_checks" ||
    Name == "nodecpp::safememory::soft_this_ptr" ||
    Name == "nodecpp::safememory::soft_this_ptr_impl" ||
    Name == "nodecpp::safememory::soft_this_ptr_no_checks";
}

bool isAwaitableName(const std::string &Name) {
  return Name == "nodecpp::awaitable";
}

bool isNakedPtrName(const std::string &Name) {
  return Name == "nodecpp::safememory::naked_ptr" ||
         Name == "nodecpp::safememory::naked_ptr_impl" ||
         Name == "nodecpp::safememory::naked_ptr_no_checks";
}

bool isConstNakedPtrName(const std::string &Name) {
  return Name == "nodecpp::safememory::const_naked_ptr";
}

bool isSystemLocation(const ClangTidyContext *Context, SourceLocation Loc) {

  auto Sm = Context->getSourceManager();
  auto ELoc = Sm->getExpansionLoc(Loc);

  return (ELoc.isInvalid() || Sm->isInSystemHeader(ELoc));
}

bool isSystemSafeTypeName(const ClangTidyContext *Context,
                      const std::string &Name) {

  //hardcode some names that are really important
  if (Name == "nodecpp::safememory::make_owning")
    return true;
  else if (isSafePtrName(Name) || isNakedPtrName(Name) ||
           isConstNakedPtrName(Name) || isAwaitableName(Name))
    return false;

  auto &Wl = Context->getGlobalOptions().SafeTypes;
  return (Wl.find(Name) != Wl.end());
}

bool isSystemSafeFunctionName(const ClangTidyContext *Context,
                      const std::string &Name) {

  //hardcode some names that are really important
  if (Name == "nodecpp::safememory::make_owning" ||
      Name == "nodecpp::wait_for_all")
    return true;

  auto &Wl = Context->getGlobalOptions().SafeFunctions;
  return (Wl.find(Name) != Wl.end());
}

bool checkNakedStructRecord(const CXXRecordDecl *Dc,
                            const ClangTidyContext *Context, DiagHelper &Dh) {

  //on debug break here
  assert(Dc);
  assert(Dc->hasDefinition());

  if (!Dc || !Dc->hasDefinition()) {
    return false;
  }

  //we check explicit and implicit here
  bool HasAttr = Dc->hasAttr<NodeCppNakedStructAttr>();

  // bool checkInits = false;
  // std::list<const FieldDecl*> missingInitializers;
  auto F = Dc->fields();
  for (auto It = F.begin(); It != F.end(); ++It) {

    auto Qt = (*It)->getType().getCanonicalType();
    if (isSafeType(Qt, Context))
      continue;

    if (auto Np = isNakedPointerType(Qt, Context)) {
      if (Np.isOk())
        continue;

      isNakedPointerType(Qt, Context, Dh); // for report
      Dh.diag((*It)->getLocation(), "unsafe type at naked_ptr declaration");
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
    Dh.diag(B.begin()->getLocStart(),
            "inheritance not allowed at naked struct");
    return false;//don't allow any bases yet
  }

  auto M = Dc->methods();
  for (auto It = M.begin(); It != M.end(); ++It) {

    auto Method = *It;

    if (isa<CXXDestructorDecl>(Method))
      continue;

    if (auto Ctr = dyn_cast<CXXConstructorDecl>(Method)) {
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

  auto Dc = Qt->getAsCXXRecordDecl();

  if (!Dc || !Dc->hasDefinition())
    return KindCheck(false, false);

  // // first verify if is a well known class,
  // auto name = decl->getQualifiedNameAsString();
  // if (isNakedStructName(name))
  //   return KindCheck(true, true);
  
  //if it has attribute, the some other rule
  // must have verified it
  if (Dc->hasAttr<NodeCppNakedStructAttr>())
    return KindCheck(true, checkNakedStructRecord(Dc, Context));

  //t->dump();
  return KindCheck(false, false);
}

FunctionKind getFunctionKind(QualType Qt) {

  if (auto Ts = Qt->getAs<TemplateSpecializationType>()) {

    auto Td = Ts->getTemplateName().getAsTemplateDecl();
    if (Td) {
      if (Td->getQualifiedNameAsString() == "nodecpp::function_owned_arg0")
        return FunctionKind::OwnedArg0;
    }

    return getFunctionKind(Ts->desugar());
  } else if (auto Rc = Qt->getAs<RecordType>()) {
    auto Dc = Rc->getDecl();
    assert(Dc);

    if (Dc->isLambda())
      return FunctionKind::Lambda;

    auto Name = Dc->getQualifiedNameAsString();
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

bool isNodecppFunctionOwnedArg0Type(QualType Qt) {

  return getFunctionKind(Qt) == FunctionKind::OwnedArg0;
}

bool isAnyFunctorType(QualType Qt) {

  auto Kind = getFunctionKind(Qt);
  return Kind == FunctionKind::StdFunction || Kind == FunctionKind::OwnedArg0;
}

bool isRawPointerType(QualType Qt) {
  assert(Qt.isCanonical());

  return Qt->isPointerType();
}

const ClassTemplateSpecializationDecl *getTemplatePtrDecl(QualType Qt) {

  assert(Qt.isCanonical());

  auto Dc = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
      Qt->getAsCXXRecordDecl());
  if (!Dc)
    return nullptr;

  if (!Dc->hasDefinition())
    return nullptr;

  auto &Args = Dc->getTemplateArgs();

  if (Args.size() < 1)
    return nullptr;

  auto &Arg0 = Args.get(0);

  if (Arg0.getKind() != TemplateArgument::Type)
    return nullptr;

  return Dc;
}

QualType getPointeeType(QualType Qt) {

  assert(Qt.isCanonical());

  if (Qt->isPointerType())
    return Qt->getPointeeType().getCanonicalType();

  auto Dc = getTemplatePtrDecl(Qt);

  assert(Dc);
  assert(Dc->hasDefinition());

  auto &Args = Dc->getTemplateArgs();

  assert(Args.size() >= 1);

  auto &Arg0 = Args.get(0);

  assert(Arg0.getKind() == TemplateArgument::Type);

  return Arg0.getAsType().getCanonicalType();
}

KindCheck isNakedPointerType(QualType Qt, const ClangTidyContext *Context,
                             DiagHelper &Dh) {

  assert(Qt.isCanonical());
  auto Dc = getTemplatePtrDecl(Qt);
  if (!Dc)
    return KindCheck(false, false);

  std::string Name = Dc->getQualifiedNameAsString();
  if (isNakedPtrName(Name) || isConstNakedPtrName(Name)) {
    QualType Pointee = getPointeeType(Qt);
    return KindCheck(true, isSafeType(Pointee, Context, Dh));
  }

  return KindCheck(false, false);
}

bool isConstNakedPointerType(QualType Qt) {

  assert(Qt.isCanonical());

  auto Dc = getTemplatePtrDecl(Qt);
  if (!Dc)
    return false;

  std::string Name = Dc->getQualifiedNameAsString();
  return isConstNakedPtrName(Name);
}

bool isSafePtrType(QualType Qt) {

  assert(Qt.isCanonical());

  auto Dc = getTemplatePtrDecl(Qt);
  if (!Dc)
    return false;

  std::string Name = Dc->getQualifiedNameAsString();
  return isSafePtrName(Name);
}

bool isAwaitableType(QualType Qt) {

  Qt = Qt.getCanonicalType();
  auto Dc = getTemplatePtrDecl(Qt);
  if (!Dc)
    return false;

  std::string Name = Dc->getQualifiedNameAsString();
  return isAwaitableName(Name);

}

bool isSafeRecord(const CXXRecordDecl *Dc, const ClangTidyContext *Context,
                  DiagHelper &Dh) {

  if (!Dc) {
    return false;
  }

  if (isSystemLocation(Context, Dc->getLocation())) {
    // if record is in system header, fate is decided by white list
    std::string Name = Dc->getQualifiedNameAsString();
    if (isSystemSafeTypeName(Context, Name)) {
      return true;
    } else {
      std::string Msg = "system library type '" + Name + "' is not safe";
      Dh.diag(Dc->getLocation(), Msg);
      return false;
    }
  }

  // if we don't have a definition, we can't check
  if (!Dc->hasDefinition())
    return false;

  if (Dc->isUnion()) {
    return checkUnion(Dc, Dh);
  }

  auto F = Dc->fields();
  for (auto It = F.begin(); It != F.end(); ++It) {
    auto Ft = (*It)->getType().getCanonicalType();
    if (!isSafeType(Ft, Context, Dh)) {
      std::string Msg =
          "member '" + std::string((*It)->getName()) + "' is not safe";
      Dh.diag((*It)->getLocation(), Msg);
      return false;
    }
  }

  auto B = Dc->bases();
  for (auto It = B.begin(); It != B.end(); ++It) {

    auto Bt = It->getType().getCanonicalType();
    if (!isSafeType(Bt, Context, Dh)) {
      Dh.diag((*It).getBaseTypeLoc(), "base class is not safe");
      return false;
    }
  }

  // finally we are safe!
  return true;
}

bool isSafeType(QualType Qt, const ClangTidyContext *Context, DiagHelper &Dh) {

  assert(Qt.isCanonical());

  if (Qt->isReferenceType()) {
    return false;
  } else if (Qt->isPointerType()) {
    return false;
  } else if (Qt->isBuiltinType()) {
    return true;
  } else if (isSafePtrType(Qt)) {
    return isSafeType(getPointeeType(Qt), Context, Dh);
  } else if (auto Rd = Qt->getAsCXXRecordDecl()) {
    return isSafeRecord(Rd, Context, Dh);
  } else if (Qt->isTemplateTypeParmType()) {
    // we will take care at instantiation
    return true;
  } else {
    //t->dump();
    return false;
  }
}

const CXXRecordDecl *isUnionType(QualType Qt) {

  assert(Qt.isCanonical());

  auto Rd = Qt->getAsCXXRecordDecl();
  return (Rd && Rd->hasDefinition() && Rd->isUnion()) ? Rd : nullptr;
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

  std::string Name = Dc->getQualifiedNameAsString();

  return isSafePtrName(Name) || isNakedPtrName(Name) ||
         isConstNakedPtrName(Name);
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
    return ignoreTemporaries(T->GetTemporaryExpr());
  } else if (auto I = dyn_cast<ImplicitCastExpr>(Ex)) {
    return ignoreTemporaries(I->getSubExpr());
  } else if (auto Cl = dyn_cast<ExprWithCleanups>(Ex)) {
    return ignoreTemporaries(Cl->getSubExpr());
  } else {
    return Ex;
  }
} // namespace nodecpp

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

bool NakedPtrScopeChecker::canArgumentGenerateOutput(QualType Out,
                                                     QualType Arg) {

  //until properly updated for naked_ptr template
  return true;
  // out.dump();
  // arg.dump();

  if (isSafeType(Arg, TidyContext))
    return false;

  if (isNakedPointerType(Arg, TidyContext))
    return true;

  if (isNakedStructType(Arg, TidyContext))
    return true;

  if (Arg->isReferenceType())
    return true;

  if (Arg->isPointerType())
    return true;

  return false;
  //TODO fix type
  // const Type *t = out.getTypePtrOrNull();
  // if (!t || !t->isPointerType())
  //   return false;

  // auto qt2 = t->getPointeeType();
  // const Type *t2 = qt2.getTypePtrOrNull();
  // if (!t2)
  //   return false;

  // const Type *targ = arg.getTypePtrOrNull();
  // if (!(targ && (targ->isPointerType() || t)))
  //   return false;

  // auto qt2arg = targ->getPointeeType();
  // const Type *t2arg = qt2arg.getTypePtrOrNull();
  // if (!t2arg)
  //   return false;

  // // assume non builtins, can generate any kind of naked pointers
  // if (!t2arg->isBuiltinType())
  //   return true;

  // if (t2arg != t2)
  //   return false;
  // else {
  //   // qt2.dump();
  //   // qt2arg.dump();
  //   return qt2.isAtLeastAsQualifiedAs(qt2arg);
  // }
}

bool NakedPtrScopeChecker::checkStack2StackAssignment(const Decl *FromDecl) {

  if(!AstContext || !OutScopeDecl) {
    Check->diag(FromDecl->getLocStart(),
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
      return ParamVar->hasAttr<NodeCppMayExtendAttr>();
    default:
      assert(false);
    }
  } else if (auto Var = dyn_cast<VarDecl>(FromDecl)) {
    if (Var->hasGlobalStorage())
      return true;
    else if (Var->hasAttr<NodeCppMayExtendAttr>()) {
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
    return checkExpr(Tmp->GetTemporaryExpr());
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

      if (ParmVar->hasAttr<NodeCppMayExtendAttr>())
        return std::make_pair(This, nullptr);
      else
        return std::make_pair(Param, nullptr);
    } else if (auto Field = dyn_cast<FieldDecl>(Dc)) {

      Ex->dumpColor();
      assert(false);
      return std::make_pair(Unknown, nullptr);
    } else if (auto Var = dyn_cast<VarDecl>(Dc)) {
      if (Var->hasGlobalStorage()) // globals can't be changed
        return std::make_pair(Unknown, nullptr);
      else if (Var->hasAttr<NodeCppMayExtendAttr>())
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
