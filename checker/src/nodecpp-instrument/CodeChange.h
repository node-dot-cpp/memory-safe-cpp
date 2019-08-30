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

#ifndef NODECPP_INSTRUMENT_CODECHANGE_H
#define NODECPP_INSTRUMENT_CODECHANGE_H

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/Tooling/Core/Replacement.h"


namespace nodecpp {




//mb: inspired from tooling::Replacements
/// However this version has an important difference.
/// It allows for order dependant changes, when trasversing AST
/// many times we have inserts from inner nodes to be at the same place
/// that inserts from outer nodes. This is order dependant but is not a
/// problem as we usually know how they should be handled.

/// Insertion code changes can bind to the left or to the right
/// of existing code. When a new insertion is done at the same place
/// we can use such binding to merge the changes as expected.
/// Two inserts at the same location, one to the left and one to the
/// right can co-exist without problem.
/// When an insert to the left is added at same location than
/// existing one, the text is added after the existing text.
/// When the same thing happends with inserts binding to the right
/// the recent text is added to the left of existing text.


class CodeChange {
  clang::FileID File;
  clang::tooling::Range ReplacementRange;
  llvm::SmallString<4> ReplacementText;
  bool ToTheLeft = false;

  friend CodeChange mergeChanges(const CodeChange &LHS, const CodeChange &RHS);
  friend CodeChange mergeHelper(const CodeChange &LHS, const CodeChange &RHS);

  CodeChange(clang::FileID File, clang::tooling::Range ReplacementRange,
    llvm::SmallString<4> ReplacementText, bool ToTheLeft = false) :
    File(File), ReplacementRange(ReplacementRange),
    ReplacementText(std::move(ReplacementText)), ToTheLeft(ToTheLeft) {}

  CodeChange(clang::FileID File, unsigned Offset, unsigned Length, llvm::StringRef Text) :
    File(File), ReplacementRange(Offset, Length), ReplacementText(Text) {}

public:
  static
  CodeChange makeReplace(const clang::SourceManager &Sources,
    clang::SourceRange Range, llvm::StringRef Text,
    const clang::LangOptions &LangOpts = clang::LangOptions());

  static
  CodeChange makeInsertLeft(const clang::SourceManager &Sources,
    clang::SourceLocation Loc, llvm::StringRef Text);

  static
  CodeChange makeInsertRight(const clang::SourceManager &Sources,
    clang::SourceLocation Loc, llvm::StringRef Text,
    const clang::LangOptions &LangOpts = clang::LangOptions());

  enum Mergeable {NONOVERLAP, MERGE, OVERLAP};
  /// Returns whether this replacement can be applied to a file.
  ///
  /// Only replacements that are in a valid file can be applied.
  bool isValid() const {
    return File.isValid();
  }

  /// Accessors.
  /// @{
  clang::FileID getFile() const { return File; }
  unsigned getOffset() const { return ReplacementRange.getOffset(); }
  unsigned getLength() const { return ReplacementRange.getLength(); }
  unsigned getEndOffset() const {
    return ReplacementRange.getOffset() + ReplacementRange.getLength();
  }
  bool toTheLeft() const { assert(isInsert()); return ToTheLeft; }
  bool isInsert() const { return getLength() == 0; }
  llvm::StringRef getReplacementText() const { return ReplacementText; }
  /// @}


  /// Applies the replacement on the Rewriter.
//  bool apply(Rewriter &Rewrite) const;

  /// Returns a human readable string representation.
  std::string toString(const clang::SourceManager &Sm) const;

  Mergeable isMergeable(const CodeChange &RHS) const;

  bool operator<(const CodeChange &RHS) const {
    if (File != RHS.File)
      return File < RHS.File;
    else if (getOffset() != RHS.getOffset())
      return getOffset() < RHS.getOffset();
    else if(ToTheLeft != RHS.ToTheLeft)
      return ToTheLeft;
    else
      return false;
  }

  bool operator==(const CodeChange &RHS) const {
    return File == RHS.File &&
          ReplacementRange == RHS.ReplacementRange &&
          ReplacementText == RHS.ReplacementText &&
          ToTheLeft == RHS.ToTheLeft;
  }
};

CodeChange mergeChanges(const CodeChange &LHS, const CodeChange &RHS);



class FileChanges {
private:
//  clang::FileID File;
  std::set<CodeChange> Replaces;

public:

  using const_iterator = std::set<CodeChange>::const_iterator;
  using const_reverse_iterator = std::set<CodeChange>::const_reverse_iterator;

  FileChanges() = default;

  /// Adds a new replacement \p R to the current set of replacements.
  /// \p R must have the same file path as all existing replacements.
  /// Returns `success` if the replacement is successfully inserted; otherwise,
  /// it returns an llvm::Error, i.e. there is a conflict between R and the
  /// existing replacements. Callers must
  /// explicitly check the Error returned, and the returned error can be
  /// converted to a string message with `llvm::toString()`.
  llvm::Error add(const CodeChange &R);

  unsigned size() const { return Replaces.size(); }

  void clear() { Replaces.clear(); }

  bool empty() const { return Replaces.empty(); }

  const_iterator begin() const { return Replaces.begin(); }

  const_iterator end() const { return Replaces.end(); }

  const_reverse_iterator rbegin() const  { return Replaces.rbegin(); }

  const_reverse_iterator rend() const { return Replaces.rend(); }

  bool operator==(const FileChanges &RHS) const {
    return Replaces == RHS.Replaces;
  }

  void applyAll(clang::Rewriter &Rewrite) const;
};


class TUChanges {
private:
  std::map<clang::FileID, FileChanges> Replaces;

public:

  using const_iterator = std::map<clang::FileID, FileChanges>::const_iterator;
  using const_reverse_iterator = std::map<clang::FileID, FileChanges>::const_reverse_iterator;

  TUChanges() = default;

  llvm::Error add(const CodeChange &R);


  unsigned size() const { return Replaces.size(); }

  void clear() { Replaces.clear(); }

  bool empty() const { return Replaces.empty(); }

  const_iterator begin() const { return Replaces.begin(); }

  const_iterator end() const { return Replaces.end(); }

  const_reverse_iterator rbegin() const  { return Replaces.rbegin(); }

  const_reverse_iterator rend() const { return Replaces.rend(); }

  bool operator==(const TUChanges &RHS) const {
    return Replaces == RHS.Replaces;
  }
};

bool overwriteChangedFiles(clang::ASTContext &Context,
  const FileChanges &Changes, llvm::StringRef ToolName);
bool overwriteChangedFiles(clang::ASTContext &Context,
  const std::map<clang::FileID, FileChanges> &Changes, llvm::StringRef ToolName);
bool overwriteChangedFiles(clang::ASTContext &Context,
  const TUChanges &Changes, llvm::StringRef ToolName);

void overwriteChangedFiles(clang::ASTContext &Context,
  const llvm::StringMap<clang::tooling::Replacements> &FileReplacements,
  llvm::StringRef Name);
void overwriteChangedFiles(clang::ASTContext &Context, 
  const clang::tooling::Replacements &FileReplacements,
  llvm::StringRef Name);


} // namespace nodecpp

#endif // NODECPP_INSTRUMENT_CODECHANGE_H

