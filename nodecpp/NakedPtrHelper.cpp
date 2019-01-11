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

namespace clang {
namespace tidy {
namespace nodecpp {

DiagHelper NullDiagHelper;

bool isSystemLocation(const ClangTidyContext* context, SourceLocation loc) {

  auto sm = context->getSourceManager();
  auto eLoc = sm->getExpansionLoc(loc);

  return (eLoc.isValid() && sm->isInSystemHeader(eLoc));
}

bool isSystemSafeName(const ClangTidyContext* context, const std::string& name) {

  auto& wl = context->getGlobalOptions().SafeTypes; 
  return (wl.find(name) != wl.end());
}


bool isOwnerPtrName(const std::string &Name) {
  return Name == "std::unique_ptr" || Name == "nodecpp::owning_ptr" || Name == "owning_ptr";
}

bool isOwningPtrRecord(const CXXRecordDecl *decl) {
  return  isOwnerPtrName(decl->getQualifiedNameAsString());
}

bool isSafePtrName(const std::string &Name) {
  return isOwnerPtrName(Name) || Name == "nodecpp::soft_ptr" ||
   Name == "nodecpp::safe_ptr" || Name == "soft_ptr";
}

bool isNakedPtrName(const std::string& name) {
  return name == "nodecpp::naked_ptr" || name == "naked_ptr";
}

bool isStdFunctionType(QualType qt) {
  
  assert(qt.isCanonical());

  auto decl = qt->getAsCXXRecordDecl();
  if (!decl || !decl->hasDefinition())
    return false;

  return decl->getQualifiedNameAsString() == "std::function";
}

bool isAnyFunctionType(QualType qt) {

  return isStdFunctionType(qt) || isNodecppFunctionOwnedArg0Type(qt);
}

bool checkNakedStructRecord(const CXXRecordDecl *decl, const ClangTidyContext* context, DiagHelper& dh) {

  //on debug break here
  assert(decl);
  assert(decl->hasDefinition());

  if (!decl || !decl->hasDefinition()) {
    return false;
  }

  //we check explicit and implicit here
  bool hasAttr = decl->hasAttr<NodeCppNakedStructAttr>();

  // bool checkInits = false;
  // std::list<const FieldDecl*> missingInitializers;
  auto f = decl->fields();
  for (auto it = f.begin(); it != f.end(); ++it) {

    auto qt = (*it)->getType().getCanonicalType(); 
    if(isSafeType(qt, context))
      continue;

    if(auto np = isNakedPointerType(qt, context)) {
      if(np.isOk())
        continue;

      isNakedPointerType(qt, context, dh); //for report
      dh.diag((*it)->getLocation(), "unsafe type at naked_ptr declaration");
      return false;
    }

    if(auto ns = isNakedStructType(qt, context)) {
      if(ns.isOk())
        continue;

      isNakedStructType(qt, context, dh); //for report
      dh.diag((*it)->getLocation(), "unsafe type at naked_struct declaration");
      return false;
    }

    //if none of the above, then is an error
    isSafeType(qt, context, dh);
    dh.diag((*it)->getLocation(), "member not allowed at naked struct");
    return false;
  }

  auto b = decl->bases();
  if(b.begin() != b.end()) {
    dh.diag(b.begin()->getLocStart(), "inheritance not allowed at naked struct");
    return false;//don't allow any bases yet
  }

  auto m = decl->methods();
  for(auto it = m.begin(); it != m.end(); ++it) {

    auto method = *it;

    if(isa<CXXDestructorDecl>(method))
      continue;

    if(auto ctr = dyn_cast<CXXConstructorDecl>(method)) {
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

    if(method->isConst())
      continue;

    if(method->isMoveAssignmentOperator() || method->isCopyAssignmentOperator()) {
      if(method->isDeleted())
        continue;

      if(hasAttr && method->isDefaulted())
        continue;
    }
    
    dh.diag(method->getLocation(), "method not allowed at naked struct");
    return false;
  }
  
  // finally we are safe!
  return true;
}


KindCheck isNakedStructType(QualType qt, const ClangTidyContext* context, DiagHelper& dh) {

  assert(qt.isCanonical());
 
  auto decl = qt->getAsCXXRecordDecl();

  if (!decl || !decl->hasDefinition())
    return KindCheck(false, false);

  // // first verify if is a well known class,
  // auto name = decl->getQualifiedNameAsString();
  // if (isNakedStructName(name))
  //   return KindCheck(true, true);
  
  //if it has attribute, the some other rule
  // must have verified it
  if(decl->hasAttr<NodeCppNakedStructAttr>())
    return KindCheck(true, checkNakedStructRecord(decl, context));

  //t->dump();
  return KindCheck(false, false);
}

bool isLambdaType(QualType qt) {
  
  assert(qt.isCanonical());
 
  if (auto decl = qt->getAsCXXRecordDecl()) {

    if (!decl || !decl->hasDefinition())
      return false;

    return decl->isLambda();
  }
  
  return false;
}

bool isNodecppFunctionOwnedArg0Type(QualType qt) {
  assert(qt.isCanonical());
 
  auto decl = qt->getAsCXXRecordDecl();

  if (!decl || !decl->hasDefinition())
    return false;

  return decl->getQualifiedNameAsString() == "nodecpp::function_owned_arg0";
}

bool isRawPointerType(QualType qt) {

  return qt.getCanonicalType()->isPointerType();
}

static
const ClassTemplateSpecializationDecl* getTemplatePtrDecl(QualType qt) {
  
  qt = qt.getCanonicalType();

  auto decl = dyn_cast_or_null<ClassTemplateSpecializationDecl>(qt->getAsCXXRecordDecl());
  if(!decl)
    return nullptr;

  if(!decl->hasDefinition())
    return nullptr;

  auto& args = decl->getTemplateArgs();
  
  if(args.size() < 1)
    return nullptr;

  auto& arg0 = args.get(0);
  
  if(arg0.getKind() != TemplateArgument::Type)
    return nullptr;

  return decl;
}

QualType getPointeeType(QualType qt) {

  assert(qt.isCanonical());

  if(qt->isPointerType())
    return qt->getPointeeType().getCanonicalType();

  auto decl = getTemplatePtrDecl(qt);

  assert(decl);
  assert(decl->hasDefinition());

  auto& args = decl->getTemplateArgs();
  
  assert(args.size() >= 1);

  auto& arg0 = args.get(0);
  
  assert(arg0.getKind() == TemplateArgument::Type);

  return arg0.getAsType().getCanonicalType();
}

QualType unwrapConstRefType(QualType qt) {

  assert(qt.isCanonical());
  if(qt->isReferenceType() && qt.isConstQualified())
    return qt->getPointeeType().getCanonicalType();

  return qt;
}


KindCheck isNakedPointerType(QualType qt, const ClangTidyContext* context, DiagHelper& dh) {
  
  auto decl = getTemplatePtrDecl(qt);
  if(!decl)
    return KindCheck(false, false);

  auto name = decl->getQualifiedNameAsString();
  if(isNakedPtrName(name)) {
    QualType pointee = getPointeeType(qt);
    return KindCheck(true, isSafeType(pointee, context, dh));
  }

  return KindCheck(false, false);
}


bool isSafePtrType(QualType qt) {

  auto decl = getTemplatePtrDecl(qt);
  if(!decl)
    return false;

  auto name = decl->getQualifiedNameAsString();
  return isSafePtrName(name);
}

bool isSafeRecord(const CXXRecordDecl *decl, const ClangTidyContext* context, DiagHelper& dh) {

  if (!decl) {
    return false;
  }

  if(isSystemLocation(context, decl->getLocation())) {
    // if record is in system header, fate is decided by white list 
    std::string name = decl->getQualifiedNameAsString();
    if(isSystemSafeName(context, name)) {
      return true;
    } else {
      dh.diag(decl->getLocation(), "system library type is not safe");
      return false;
    }
  }

  // if we don't have a definition, we can't check
  if(!decl->hasDefinition())
    return false;

  if(decl->isUnion()) {
    return checkUnion(decl, dh);
  }

  auto F = decl->fields();
  for (auto It = F.begin(); It != F.end(); ++It) {
    auto ft = (*It)->getType().getCanonicalType();
    if (!isSafeType(ft, context, dh)) {
      dh.diag((*It)->getLocation(), "member is not safe");
      return false;
    }
  }

  auto B = decl->bases();
  for (auto It = B.begin(); It != B.end(); ++It) {

    auto bt = It->getType().getCanonicalType();
    if (!isSafeType(bt, context, dh)) {
      dh.diag((*It).getBaseTypeLoc(), "base class is not safe");
      return false;
    }
  }

  // finally we are safe!
  return true;
}


bool isSafeType(QualType qt, const ClangTidyContext* context, DiagHelper& dh) {

  assert(qt.isCanonical());

  if (qt->isReferenceType()) {
    return false;
  } else if(qt->isPointerType()) {
    return false;
  } else if (qt->isBuiltinType()) {
    return true;
  } else if(isSafePtrType(qt)) {
    return isSafeType(getPointeeType(qt), context, dh);
  } else if (auto rd = qt->getAsCXXRecordDecl()) {
    return isSafeRecord(rd, context, dh);
  } else if (qt->isTemplateTypeParmType()) {
    // we will take care at instantiation
    return true;
  } else {
    //t->dump();
    return false;
  }
}

const CXXRecordDecl* isUnionType(QualType qt) {

  assert(qt.isCanonical());

  auto rd = qt->getAsCXXRecordDecl();
  return (rd && rd->hasDefinition() && rd->isUnion()) ? rd : nullptr;
}

bool checkUnion(const CXXRecordDecl *decl, DiagHelper& dh) {

  assert(decl);
  assert(decl->hasDefinition());
  assert(decl->isUnion());


  auto f = decl->fields();
  for (auto it = f.begin(); it != f.end(); ++it) {

      auto qt = (*it)->getType().getCanonicalType();
      if(isRawPointerType(qt)) {

        dh.diag((*it)->getLocation(), "(S1.4) raw pointers inside unions are prohibited");
        return false;
      }
      else if(!qt->isBuiltinType() && !qt->isTemplateTypeParmType()) {

        dh.diag((*it)->getLocation(), "(S1.4) non-primitives inside unions are prohibited");
        return false;
      }
  }

  // finally we are safe!
  return true;
}

bool isOsnPtrRecord(const CXXRecordDecl *decl) {
  std::string name = decl->getQualifiedNameAsString();

  return isSafePtrName(name) || isNakedPtrName(name);
}

const Expr* getBaseIfOsnPtrDerref(const Expr* expr) {

  if(!expr)
    return nullptr;

  if(auto opCall = dyn_cast<CXXOperatorCallExpr>(expr)) {
    if(auto opDecl = dyn_cast<CXXMethodDecl>(opCall->getDirectCallee())) {
      if(isOsnPtrRecord(opDecl->getParent())) {
        auto name = opDecl->getDeclName();
        if(name.getNameKind() == DeclarationName::CXXOperatorName) {
          auto op = name.getCXXOverloadedOperator();
          if(op == OO_Star || op == OO_Arrow) {
            assert(opCall->getNumArgs() > 0);
            return opCall->getArg(0);
          }
        }
      }
    }
  }
  //TODO check for method get()
  return nullptr;
}

const Expr *getParentExpr(ASTContext *context, const Expr *expr) {

  auto sList = context->getParents(*expr);

  auto sIt = sList.begin();

  if (sIt == sList.end())
    return nullptr;

  auto p = sIt->get<Expr>();
  if(!p)
    return nullptr;
    
  if (isa<ParenExpr>(p) || isa<ImplicitCastExpr>(p) 
    || isa<MaterializeTemporaryExpr>(p))
    return getParentExpr(context, p);
  else
    return p;
}

const Expr *ignoreTemporaries(const Expr *expr) {
  if (auto b = dyn_cast<CXXBindTemporaryExpr>(expr)) {
    return ignoreTemporaries(b->getSubExpr());
  }else if (auto c = dyn_cast<CXXConstructExpr>(expr)) {
    return ignoreTemporaries(c->getArg(0));
  } else if (auto t = dyn_cast<MaterializeTemporaryExpr>(expr)) {
    return ignoreTemporaries(t->GetTemporaryExpr());
  } else if (auto i = dyn_cast<ImplicitCastExpr>(expr)) {
    return ignoreTemporaries(i->getSubExpr());
  } else if (auto cl = dyn_cast<ExprWithCleanups>(expr)) {
    return ignoreTemporaries(cl->getSubExpr());
  } else {
	  return expr;
  }
} // namespace nodecpp

bool isFunctionPtr(const Expr *expr) {

  if (!expr)
    return false;

  auto e = ignoreTemporaries(expr);

  if (auto op = dyn_cast<UnaryOperator>(e)) {
    if (op->getOpcode() != UnaryOperatorKind::UO_AddrOf)
      return false;

    e = op->getSubExpr();
  }

  if (auto ref = dyn_cast<DeclRefExpr>(e)) {
    return isa<FunctionDecl>(ref->getDecl());
  }

  return false;
}



const Stmt *getParentStmt(ASTContext *context, const Stmt *stmt) {

  auto sList = context->getParents(*stmt);

  auto sIt = sList.begin();

  if (sIt != sList.end())
    return sIt->get<Stmt>();
  else
    return nullptr;
}


const DeclStmt* getParentDeclStmt(ASTContext *context, const Decl* decl) {

  if(!decl)
    return nullptr;

  auto l = context->getParents(*decl);

  if(l.begin() != l.end())
    return l.begin()->get<DeclStmt>();
  else
    return nullptr;
}

bool NakedPtrScopeChecker::canArgumentGenerateOutput(QualType out, QualType arg) {

  //until properly updated for naked_ptr template
  return true;
  // out.dump();
  // arg.dump();

  if(isSafeType(arg, tidyContext))
    return false;

  if(isNakedPointerType(arg, tidyContext))
    return true;
  
  if(isNakedStructType(arg, tidyContext))
    return true;
    
  if(arg->isReferenceType())
    return true;

  if(arg->isPointerType())
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

bool NakedPtrScopeChecker::checkStack2StackAssignment(const Decl *fromDecl) {

  if(!astContext || !outScopeDecl) {
    check->diag(fromDecl->getLocStart(), "Internal checker error, please report", DiagnosticIDs::Error);
    return false;
  }
  
  auto fromStmt = getParentDeclStmt(astContext, fromDecl);
  if (!fromStmt)
    return false;

  auto toStmt = getParentDeclStmt(astContext, outScopeDecl);
  if (!toStmt)
    return false;

  auto from = getParentStmt(astContext, fromStmt);
  if (!from)
    return false;

  auto to = getParentStmt(astContext, toStmt);
  while (to) {
    if (to == from)
      return true;

    to = getParentStmt(astContext, to);
  }

  // we couldn't verify this is ok, assume the worst
  return false;
}

bool NakedPtrScopeChecker::checkDeclRefExpr(const DeclRefExpr *declRef) {
  auto fromDecl = declRef->getDecl();
  if (!fromDecl) { // shouln't happend here
    return false;
  }

  if (isa<FieldDecl>(fromDecl)) {
    fromDecl->dumpColor();
    assert(false);
  } else if (auto paramVar = dyn_cast<ParmVarDecl>(fromDecl)) {
    switch (outScope) {
    case Stack:
    case Param:
      return true;
    case This:
      return paramVar->hasAttr<NodeCppMayExtendAttr>();
    default:
      assert(false);
    }
  } else if (auto var = dyn_cast<VarDecl>(fromDecl)) {
    if (var->hasGlobalStorage())
      return true;
    else if(var->hasAttr<NodeCppMayExtendAttr>()) {
      return true;
    }
    else {
      if (outScope == Stack) {
        return checkStack2StackAssignment(fromDecl);
      }
      return false;
    }
  }

  return false;
}

bool NakedPtrScopeChecker::checkCallExpr(const CallExpr *call) {

  if(!call) { // shouln't happend here
    assert(false);
    return false;
  }
    
  auto callDecl = call->getDirectCallee();
  if (!callDecl) {
    return false;
  }

  if(isa<CXXMemberCallExpr>(call)) {
    auto callee = dyn_cast<MemberExpr>(call->getCallee());

    if(!callee) {
      //can this happend?
      return false;
    }

    if(isSafePtrType(callee->getBase()->getType()))
      return true;

    if(!checkExpr(callee->getBase())) {
        return false;
    }
  }
  // TODO check if callee is not the first argument

  auto params = callDecl->parameters();
  auto ret = callDecl->getReturnType().getCanonicalType();
  auto args = call->arguments();

  auto it = args.begin();
  auto jt = params.begin();

  if(isa<CXXOperatorCallExpr>(call) && isa<CXXMethodDecl>(callDecl)) {
    // this is an instance method operator call
    // then first argument is actually the instance
    if (it == args.end()) {
      assert(false);
      return false;
    }

    if(isSafePtrType((*it)->getType()))
      return true;

    if (!checkExpr(*it)) {
      return false;
    }
    ++it;
  }

  while (it != args.end() && jt != params.end()) {
    auto arg = (*jt)->getType().getCanonicalType();
    if (canArgumentGenerateOutput(ret, arg)) {
      // diag((*it)->getExprLoc(),
      //     "check this argument");

      if (!checkExpr(*it)) {
        return false;
      }
    }
    ++it;
    ++jt;
  }
  if (it != args.end() || jt != params.end()) {
    //TODO diag
    return false;
  }

  // this is ok!
  return true;
}

bool NakedPtrScopeChecker::checkCXXConstructExpr(const CXXConstructExpr *construct) {

  auto args = construct->arguments();

  for(auto it = args.begin(); it != args.end(); ++it) {

    if (!checkExpr(*it)) {
      return false;
    }
  }

  // this is ok!
  return true;

}


bool NakedPtrScopeChecker::checkExpr(const Expr *from) {

  if(outScope == Unknown) {
    return false;
  }

  if(!from) { // shouln't happend here
    assert(false);
    return false;
  }

  from = from->IgnoreParenImpCasts();
  if (isa<CXXThisExpr>(from)) {
    switch (outScope) {
    case Stack:
    case Param:
    case This:
      return true;
    default:
      assert(false);
      return false;
    }
  } else if (isa<CXXNullPtrLiteralExpr>(from)) {
    return true;
  } else if (isa<IntegerLiteral>(from)) {
    return true;
  } else if (auto declRef = dyn_cast<DeclRefExpr>(from)) {
    return checkDeclRefExpr(declRef);
  } else if (auto callExpr = dyn_cast<CallExpr>(from)) {
    // this will check functions, members and both operators
    return checkCallExpr(callExpr);
  } else if (auto member = dyn_cast<MemberExpr>(from)) {
    // TODO verify only members and not methods will get in here
    return checkExpr(member->getBase());
  } else if (auto op = dyn_cast<UnaryOperator>(from)) {
    return checkExpr(op->getSubExpr());
  } else if (auto op = dyn_cast<BinaryOperator>(from)) {
    if(checkExpr(op->getLHS()))
      return checkExpr(op->getRHS());
    else    
      return false;
  } else if (auto defArg = dyn_cast<CXXDefaultArgExpr>(from)) {
    return checkExpr(defArg->getExpr());
  } else if (auto op = dyn_cast<ConditionalOperator>(from)) {
    //check both branches
    if(checkExpr(op->getTrueExpr()))
      return checkExpr(op->getFalseExpr());
    else    
      return false;
  } else if(auto cast = dyn_cast<CastExpr>(from)) {
    return checkExpr(cast->getSubExpr());
  } else if(auto tmp = dyn_cast<MaterializeTemporaryExpr>(from)) {
    return checkExpr(tmp->GetTemporaryExpr());
  } else if(auto construct = dyn_cast<CXXConstructExpr>(from)) {
    return checkCXXConstructExpr(construct);
  }


  //just in case
  from->dumpColor();
  return false;
}


/* static */
std::pair<NakedPtrScopeChecker::OutputScope, const Decl*>
NakedPtrScopeChecker::calculateScope(const Expr *expr) {

  // Scope is only calculated for the expression acting as lhs
  // all other expressions are checked againt them
  if(!expr) {
    assert(expr);
    return std::make_pair(Unknown, nullptr);
  }

  expr = expr->IgnoreParenImpCasts();
  if (auto declRef = dyn_cast<DeclRefExpr>(expr)) {
    auto decl = declRef->getDecl();
    if (!decl) { // shouldn't happend here
      return std::make_pair(Unknown, nullptr);
    }

    if (auto parmVar = dyn_cast<ParmVarDecl>(decl)) {

      if (parmVar->hasAttr<NodeCppMayExtendAttr>())
        return std::make_pair(This, nullptr);
      else
        return std::make_pair(Param, nullptr);
    } else if (auto field = dyn_cast<FieldDecl>(decl)) {

      expr->dumpColor();
      assert(false);
      return std::make_pair(Unknown, nullptr);
    } else if (auto var = dyn_cast<VarDecl>(decl)) {
      if (var->hasGlobalStorage()) //globals can't be changed
        return std::make_pair(Unknown, nullptr);
      else if(var->hasAttr<NodeCppMayExtendAttr>())
        return std::make_pair(This, nullptr);
      else
        return std::make_pair(Stack, decl);
    }
  } else if (auto member = dyn_cast<MemberExpr>(expr)) {
    // TODO verify only members and not methods will get in here
    // TODO also verify is a hard member and not a pointer / unique_ptr
    return calculateScope(member->getBase());
  } else if (isa<CXXThisExpr>(expr)) {
    return std::make_pair(This, nullptr);
  }

  llvm::errs() << "NakedPtrScopeChecker::calculateScope > Unknown\n";
  expr->dumpColor();
  return std::make_pair(Unknown, nullptr);
}

/* static */
NakedPtrScopeChecker NakedPtrScopeChecker::makeChecker(ClangTidyCheck *check, ClangTidyContext* tidyContext,
                                                       ASTContext *astContext,
                                                       const Expr *toExpr) {

  auto sc = NakedPtrScopeChecker::calculateScope(toExpr);

  return NakedPtrScopeChecker(check, tidyContext, astContext, sc.first, sc.second);
}

/* static */
NakedPtrScopeChecker NakedPtrScopeChecker::makeThisScopeChecker(ClangTidyCheck *check, ClangTidyContext* tidyContext) {

  return NakedPtrScopeChecker(check, tidyContext, nullptr, This, nullptr);
}
/* static */
NakedPtrScopeChecker NakedPtrScopeChecker::makeParamScopeChecker(ClangTidyCheck *check, ClangTidyContext* tidyContext) {

  return NakedPtrScopeChecker(check, tidyContext, nullptr, Param, nullptr);
}

} // namespace tidy
} // namespace clang
} // namespace clang
