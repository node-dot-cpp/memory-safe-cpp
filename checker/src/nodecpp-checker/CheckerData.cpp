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

bool CheckerData::isHeapSafe(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = SafeTypes.find(T);
  
  if(It != SafeTypes.end())
    return It->second.isSafe;;

  TypeChecker Tc(Context, NullDiagHelper);

  bool S = Tc.isSafeType(Qt);
  SafeTypes.insert(std::make_pair(T, SafeData(S)));

  return S;
}

void CheckerData::reportNonSafeDetail(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = SafeTypes.find(T);
  
  assert (It != SafeTypes.end());

  if(It->second.wasReported)
    return;

  It->second.wasReported = true;

  DiagHelper Dh(Context);

  TypeChecker Tc(Context, Dh);

  bool S = Tc.isSafeType(Qt);
  assert(!S);
}

bool CheckerData::isNullablePtr(QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = NakedPointerTypes.find(T);
  
  if(It != NakedPointerTypes.end())
    return It->second.isKind;

  auto Ck = isNakedPointerType(Qt, Context);
  NakedPointerTypes.insert(std::make_pair(T, NakedPointerData(static_cast<bool>(Ck), Ck.isOk())));

  return static_cast<bool>(Ck);
}

KindCheck2 CheckerData::checkNullablePtr(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = NakedPointerTypes.find(T);
  
  if(It != NakedPointerTypes.end())
    return KindCheck2(It->second.isKind, It->second.checkOk);

  auto Ck = isNakedPointerType(Qt, Context);
  NakedPointerTypes.insert(std::make_pair(T, NakedPointerData(static_cast<bool>(Ck), Ck.isOk())));

  return KindCheck2(static_cast<bool>(Ck), Ck.isOk());
}

void CheckerData::reportNullablePtrDetail(clang::QualType Qt) {

  Qt = Qt.getCanonicalType();
  const clang::Type* T = Qt.getTypePtr();
  auto It = NakedPointerTypes.find(T);
  
  assert (It != NakedPointerTypes.end());

  if(It->second.wasReported)
    return;

  It->second.wasReported = true;

  DiagHelper Dh(Context);

  auto Ck = isNakedPointerType(Qt, Context, Dh);

  assert(static_cast<bool>(Ck));
  assert(!Ck.isOk());
}



} // namespace checker
} // namespace nodecpp
