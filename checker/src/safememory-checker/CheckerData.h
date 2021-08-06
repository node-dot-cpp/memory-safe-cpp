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

#ifndef NODECPP_CHECKER_CHECKERDATA_H
#define NODECPP_CHECKER_CHECKERDATA_H

#include "clang/AST/Type.h"
#include <set>
#include <map>
#include <string>

namespace nodecpp {
namespace checker {

class ClangTidyContext;


/// \brief Helper class that has information about a type meeting certain criteria
/// and also where there is some error in the type or not.
class KindCheck2 {
  bool IsKind = false;
  bool CheckOk = false;
public:
  KindCheck2(bool IsKind, bool CheckOk)
    :IsKind(IsKind), CheckOk(CheckOk) {}
  operator bool () const { return IsKind; }
  bool isOk() const { return CheckOk; }
};

/// \brief Contains checker collected information that is persistent
/// acrosss several rules.
class CheckerData {
public:

  void setContext(ClangTidyContext* Context) {
    this->Context = Context;
  }

  /// \brief Adds a namespace to the set of unsafe namespaces.
  void addUnsafeNamespace(const std::string& Name);

  /// \brief Returns \c true if \p Name has an unsafe namespace as prefix.
  bool isFromUnsafeNamespace(const std::string& Name) const;

  /// \brief Returns \c true if function is \c [[no_side_effect]]
  bool isNoSideEffect(const clang::FunctionDecl *D);

  /// \brief Returns \c true if type is safe.
  bool isHeapSafe(clang::QualType Qt);
  void reportNonSafeDetail(clang::QualType Qt);

  /// \brief Returns \c true if type is 'stack only' system type.
  bool isStackOnlyIterator(clang::QualType Qt);

  /// \brief Returns \c true if type any 'stack only' type.
  bool isStackOnly(clang::QualType Qt);

  /// \brief Returns \c true if type is deterministic.
  bool isDeterministic(clang::QualType Qt);
  void reportDeterministicDetail(clang::QualType Qt);

  /// \brief Returns \c true if type is a \c nullable_ptr type.
  bool isNullablePtr(clang::QualType Qt) {
    return static_cast<bool>(checkNullablePtr(Qt));
  }
  KindCheck2 checkNullablePtr(clang::QualType Qt);
  void reportNullablePtrDetail(clang::QualType Qt);

  /// \brief Returns \c true if type is a \c [[naked_struct]] class.
  bool isNakedStruct(clang::QualType Qt) {
    return static_cast<bool>(checkNakedStruct(Qt));
  }
  KindCheck2 checkNakedStruct(clang::QualType Qt);
  void reportNakedStructDetail(clang::QualType Qt);

  /// \brief Returns \c true if type is a \c [[deep_const]] class.
  bool isDeepConst(clang::QualType Qt) {
    return static_cast<bool>(checkDeepConst(Qt));
  }
  KindCheck2 checkDeepConst(clang::QualType Qt);
  void reportDeepConstDetail(clang::QualType Qt);

  bool isSystemLocation(const clang::SourceManager* Sm, clang::SourceLocation Loc);

private:

  std::set<std::string> UnsafeNamespaces;
  std::set<const clang::FunctionDecl*> NoSideEffectFuncs;
  std::map<clang::FileID, bool> SystemFiles;

  struct TypeData2 {
    bool wasTested = false;
    bool isPositive = false;
    bool isOk = false;
    bool wasReported = false;
  };

  struct TypeData {
    //implicit attribute
    TypeData2 isHeapSafe;
    TypeData2 isStackOnly;
    TypeData2 isDeterministic;

    // explicit attributes
    TypeData2 isNullablePtr;
    TypeData2 isNakedStruct;
    TypeData2 isDeepConst;
  };

  std::map<const clang::Type*, TypeData> Data;

  ClangTidyContext* Context = nullptr;
};

typedef CheckerData CheckHelper;

} // end namespace checker
} // end namespace nodecpp

#endif // NODECPP_CHECKER_CHECKERDATA_H
