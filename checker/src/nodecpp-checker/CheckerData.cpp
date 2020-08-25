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

#include "CheckerData.h"

#include "nodecpp/NakedPtrHelper.h"

namespace nodecpp {
namespace checker {



void CheckerData::addUnsafeNamespace(const std::string& Name) {
  UnsafeNamespaces.insert(Name + "::");
}

bool CheckerData::isFromUnsafeNamespace(const std::string& Name) const {

  for(auto& Each : UnsafeNamespaces) {
    if(Name.compare(0, Each.size(), Each) == 0) {
      return true;
    }
  }
  return false;
}



bool CheckerData::isNoSideEffect(const clang::FunctionDecl *D) {
  
  if(!D)
    return false;

  D = D->getCanonicalDecl();
  if(NoSideEffectFuncs.find(D) != NoSideEffectFuncs.end())
    return true;

  if(D->getTemplateInstantiationPattern()) {
    D = D->getTemplateInstantiationPattern();
  }

  std::string Name = getQnameForSystemSafeDb(D);
  if(isStdMoveOrForward(Name)) {
    NoSideEffectFuncs.insert(D);
    return true;
  }

  if(D->hasAttr<NodeCppNoSideEffectAttr>()) {
    NoSideEffectFuncs.insert(D);
    return true;
  }

  if(D->hasAttr<SafeMemoryNoSideEffectAttr>()) {
    NoSideEffectFuncs.insert(D);
    return true;
  }

  auto M = dyn_cast<CXXMethodDecl>(D);
  if(M) {
    if(M->isConst() &&
        M->getParent()->hasAttr<NodeCppNoSideEffectWhenConstAttr>()) {
      NoSideEffectFuncs.insert(D);
      return true;
    }
    if(M->isConst() &&
        M->getParent()->hasAttr<SafeMemoryNoSideEffectWhenConstAttr>()) {
      NoSideEffectFuncs.insert(D);
      return true;
    }
    //make implicit when std::hash and std::equal_to on operator()
    // if(M->isOverloadedOperator() && M->getOverloadedOperator() == OO_Call) {
    //   auto Rd = M->getParent();
    //   std::string Name = getQnameForSystemSafeDb(Rd);
    //   if(isStdHashOrEqualToName(Name)) {
    //     NoSideEffectFuncs.insert(D);
    //     return true;
    //   }
    // }
  }

  return false;
}


bool CheckerData::isHeapSafe(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();

  auto& V = Data[T];

  if(!V.isHeapSafe.wasTested) {

    TypeChecker Tc(Context, NullDiagHelper);
    bool S = Tc.isSafeType(Qt);

    V.isHeapSafe.wasTested = true;
    V.isHeapSafe.isPositive = S;
    V.isHeapSafe.isOk = S;
  }

  return V.isHeapSafe.isPositive;
}

void CheckerData::reportNonSafeDetail(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = Data.find(T);
  
  assert (It != Data.end());

  if(It->second.isHeapSafe.wasReported)
    return;

  It->second.isHeapSafe.wasReported = true;

  DiagHelper Dh(Context);

  TypeChecker Tc(Context, Dh);

  Tc.isSafeType(Qt);
// /  assert(!S);
}

bool CheckerData::isDeterministic(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();

  auto& V = Data[T];

  if(!V.isDeterministic.wasTested) {

    TypeChecker Tc(Context, NullDiagHelper);
    bool S = Tc.isDeterministicType(Qt);

    V.isDeterministic.wasTested = true;
    V.isDeterministic.isPositive = S;
    V.isDeterministic.isOk = S;
  }

  return V.isDeterministic.isPositive;

}


void CheckerData::reportDeterministicDetail(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = Data.find(T);
  
  assert (It != Data.end());

  if(It->second.isDeterministic.wasReported)
    return;

  It->second.isDeterministic.wasReported = true;

  DiagHelper Dh(Context);

  TypeChecker Tc(Context, Dh);

  Tc.isDeterministicType(Qt);
}


KindCheck2 CheckerData::checkNullablePtr(clang::QualType Qt) {


  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();

  auto& V = Data[T];

  if(!V.isNullablePtr.wasTested) {
    auto Ck = isNakedPointerType(Qt, Context);
    V.isNullablePtr.wasTested = true;
    V.isNullablePtr.isPositive = static_cast<bool>(Ck);
    V.isNullablePtr.isOk = Ck.isOk();
  }

  return KindCheck2(V.isNullablePtr.isPositive, V.isNullablePtr.isOk);
}

void CheckerData::reportNullablePtrDetail(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = Data.find(T);
  
  assert (It != Data.end());

  if(It->second.isNullablePtr.wasReported)
    return;

  It->second.isNullablePtr.wasReported = true;

  DiagHelper Dh(Context);

  auto Ck = isNakedPointerType(Qt, Context, Dh);

  assert(static_cast<bool>(Ck));
  assert(!Ck.isOk());
}

KindCheck2 CheckerData::checkNakedStruct(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();

  auto& V = Data[T];

  if(!V.isNakedStruct.wasTested) {
    auto Ck = isNakedStructType(Qt, Context);
    V.isNakedStruct.wasTested = true;
    V.isNakedStruct.isPositive = static_cast<bool>(Ck);
    V.isNakedStruct.isOk = Ck.isOk();
  }

  return KindCheck2(V.isNakedStruct.isPositive, V.isNakedStruct.isOk);
}

void CheckerData::reportNakedStructDetail(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = Data.find(T);
  
  assert (It != Data.end());

  if(It->second.isNakedStruct.wasReported)
    return;

  It->second.isNakedStruct.wasReported = true;

  DiagHelper Dh(Context);

  auto Ck = isNakedStructType(Qt, Context, Dh);

  assert(static_cast<bool>(Ck));
  assert(!Ck.isOk());
}

KindCheck2 CheckerData::checkDeepConst(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();

  auto& V = Data[T];

  if(!V.isDeepConst.wasTested) {
    auto Ck = isDeepConstType(Qt, Context);
    V.isDeepConst.wasTested = true;
    V.isDeepConst.isPositive = static_cast<bool>(Ck);
    V.isDeepConst.isOk = Ck.isOk();
  }

  return KindCheck2(V.isDeepConst.isPositive, V.isDeepConst.isOk);
}

void CheckerData::reportDeepConstDetail(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = Data.find(T);
  
  assert (It != Data.end());

  if(It->second.isDeepConst.wasReported)
    return;

  It->second.isDeepConst.wasReported = true;

  DiagHelper Dh(Context);

  auto Ck = isDeepConstType(Qt, Context, Dh);

  assert(static_cast<bool>(Ck));
  assert(!Ck.isOk());
}


} // namespace checker
} // namespace nodecpp
