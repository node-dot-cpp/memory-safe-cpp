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

#ifndef NODECPP_INSTRUMENT_DEZOMBIEFYHELPER_H
#define NODECPP_INSTRUMENT_DEZOMBIEFYHELPER_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"

namespace nodecpp {


inline
bool isInSystemHeader(clang::ASTContext &Context, clang::Decl *D) {
  
  if (!llvm::isa<clang::TranslationUnitDecl>(D)) {

    auto &SourceManager = Context.getSourceManager();
    auto ExpansionLoc = SourceManager.getExpansionLoc(D->getLocation());
    if (ExpansionLoc.isInvalid()) {
      return true;
    }
    if (SourceManager.isInSystemHeader(ExpansionLoc)) {
      return true;
    }
  }
  return false;
}


inline
bool needsExternalDezombiefy(clang::ASTContext &Context, clang::CXXRecordDecl *D) {
  return isInSystemHeader(Context, D);
}

inline
bool needsExternalDezombiefy(clang::ASTContext &Context, clang::FunctionDecl *D) {
  return isInSystemHeader(Context, D);
}

inline
bool isByValueUserTypeNonTriviallyCopyable(clang::ASTContext &Context, clang::QualType Qt) {
  Qt = Qt.getCanonicalType();
  if(Qt->isStructureOrClassType()) {
//    Qt.dump();
    auto Rd = Qt->getAsCXXRecordDecl();
    bool SysType = isInSystemHeader(Context, Rd);
    if(!SysType && !Rd->isTriviallyCopyable()) {
      return true;
    }
  }
  return false;
}

inline
bool mayZombie(clang::QualType Qt) {
  Qt = Qt.getCanonicalType();
  return Qt->isLValueReferenceType() || Qt->isPointerType();
}

inline
bool isDezombiefyCandidate(clang::DeclRefExpr *E) {
  if(auto D = E->getDecl())
    return mayZombie(D->getType());

  return false;
}

inline
bool isLiteralExpr(const clang::Expr *E) {
  return llvm::isa<clang::IntegerLiteral>(E) || llvm::isa<clang::StringLiteral>(E) ||
    llvm::isa<clang::CXXNullPtrLiteralExpr>(E) || llvm::isa<clang::FloatingLiteral>(E);
}

inline
const clang::Expr *getParentExpr(clang::ASTContext &Context, const clang::Expr *Ex) {

  auto SList = Context.getParents(*Ex);

  auto SIt = SList.begin();

  if (SIt == SList.end())
    return nullptr;

  return SIt->get<clang::Expr>();
}


} // namespace nodecpp

#endif // NODECPP_INSTRUMENT_DEZOMBIEFYHELPER_H

