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

#include "CodeChange.h"

#include "clang/Basic/SourceManager.h"
#include "clang/Basic/FileManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"

namespace nodecpp {

using namespace std;
using namespace llvm;
using namespace clang;
using namespace clang::tooling;


clang::CharSourceRange toCheckedCharRange(const clang::SourceRange &Source,
  const clang::SourceManager &Sm, const clang::LangOptions &LangOpts) {


    //TODO add some MACRO support
    if(Source.getBegin().isMacroID() || Source.getEnd().isMacroID())
      return {};

    auto Ch = CharSourceRange::getTokenRange(Source);
    auto Ch2 = Lexer::makeFileCharRange(Ch, Sm, LangOpts);
    if(Ch2.isInvalid())
      return {};

    if (Ch2.isTokenRange()) {
      unsigned length = Lexer::MeasureTokenLength(Ch2.getEnd(), Sm, LangOpts);
      Ch2.setEnd(Ch2.getEnd().getLocWithOffset(length));
    }

    FileID F1 = Sm.getFileID(Ch2.getBegin());
    FileID F2 = Sm.getFileID(Ch2.getEnd());
    
    if (F1 != F2)
      return {};

    return Ch2;
  }


CodeChange CodeChange::makeReplace(const SourceManager &Sources,
  CharSourceRange ChRange, StringRef Text, const LangOptions &LangOpts) {

  assert(ChRange.isValid());

  std::pair<FileID, unsigned> Start = Sources.getDecomposedLoc(ChRange.getBegin());
  std::pair<FileID, unsigned> End = Sources.getDecomposedLoc(ChRange.getEnd());

  assert(ChRange.isCharRange());
  assert(Start.first == End.first);

  unsigned Length = End.second - Start.second;

  return {Start.first, {Start.second, Length}, Text};
}

CodeChange CodeChange::makeInsertLeft(const SourceManager &Sources,
  SourceLocation Loc, StringRef Text) {

  assert(Loc.isValid());

  std::pair<FileID, unsigned> Start = Sources.getDecomposedLoc(Loc);

  return {Start.first, {Start.second, 0}, Text, false};
}

CodeChange CodeChange::makeInsertRight(const SourceManager &Sources,
  SourceLocation Loc, StringRef Text, const LangOptions &LangOpts) {

  assert(Loc.isValid());

  std::pair<FileID, unsigned> End = Sources.getDecomposedLoc(Loc);
  End.second += Lexer::MeasureTokenLength(Loc, Sources, LangOpts);

  return {End.first, {End.second, 0}, Text, true};
}

std::string CodeChange::toString(const clang::SourceManager &Sm) const {
  std::string Result;
  llvm::raw_string_ostream Stream(Result);
  const FileEntry *Entry = Sm.getFileEntryForID(getFile());

  Stream << (Entry ? Entry->getName() : "<unknown>") << ": " << getOffset() << ":+"
         << getLength() << ":\"" << getReplacementText() << "\"";
  return Stream.str();
}

CodeChange::Mergeable CodeChange::isMergeable(const CodeChange &RHS) const {
  if(isInsert() && RHS.isInsert())
    return (getOffset() == RHS.getOffset() && ToTheLeft == RHS.ToTheLeft) ? MERGE : NONOVERLAP;
  else if(isInsert()) {
    if(getOffset() == RHS.getEndOffset())
      return ToTheLeft ? MERGE : NONOVERLAP;
    else if(getOffset() == RHS.getOffset())
      return !ToTheLeft ? MERGE : NONOVERLAP;
    else
      return ReplacementRange.overlapsWith(RHS.ReplacementRange) ? OVERLAP : NONOVERLAP;
  }
  else if(RHS.isInsert()) {
    return RHS.isMergeable(*this);
  }
  else {
    return ReplacementRange.overlapsWith(RHS.ReplacementRange) ? OVERLAP : NONOVERLAP;
  }
}

CodeChange mergeHelper(const CodeChange &LHS, const CodeChange &RHS) {
    
    assert(LHS.isInsert());
    assert(!RHS.isInsert());
    
    if(LHS.toTheLeft()) {
      // RHS must end where LHS is
      assert(RHS.getEndOffset() == LHS.getOffset());

      SmallString<4> R = RHS.ReplacementText.str();
      R += LHS.ReplacementText.str();
      return CodeChange{RHS.File, RHS.ReplacementRange, std::move(R)};
    }
    else {
      assert(LHS.getOffset() == RHS.getOffset());
      SmallString<4> R = LHS.ReplacementText.str();
      R + RHS.ReplacementText.str();
      return CodeChange{LHS.File, LHS.ReplacementRange, std::move(R)};
    }
}

CodeChange mergeChanges(const CodeChange &LHS, const CodeChange &RHS) {

  if(LHS.isInsert() && RHS.isInsert()) {
    //then both must be at the same place and to the same side
    assert(LHS.getOffset() == RHS.getOffset());
    assert(LHS.toTheLeft() == RHS.toTheLeft());
    if(LHS.toTheLeft()) {
      SmallString<4> R = LHS.ReplacementText.str();
      R += RHS.ReplacementText.str();
      return CodeChange{LHS.File, LHS.ReplacementRange, std::move(R), LHS.ToTheLeft};
    }
    else {//is to the right
      SmallString<4> R = RHS.ReplacementText.str();
      R += LHS.ReplacementText.str();
      return CodeChange{LHS.File, LHS.ReplacementRange, std::move(R), LHS.ToTheLeft};
    }
  }
  else if(LHS.isInsert())
    return mergeHelper(LHS, RHS);
  else
    return mergeHelper(RHS, LHS);
}

//  static
char CodeChangeError::ID = 0;

CodeChangeError::CodeChangeError(ErrorCode Err, const clang::SourceManager &Sm,
    const CodeChange &ExistingChange, const CodeChange &NewChange) : Err(Err) {
      switch (Err) {
      case ErrorCode::fail_to_apply:
        Message = "Failed to apply a replacement.";
        break;
      case ErrorCode::wrong_file_path:
        Message = "The new replacement's file path is different from the file path of "
              "existing replacements";
        break;
      case ErrorCode::overlap_conflict:
        Message = "The new replacement overlaps with an existing replacement.";
        break;
      default:
        llvm_unreachable("A value of replacement_error has no message.");
      }

      Message += "\nNew: " + NewChange.toString(Sm);
      Message += "\nExisting: " + ExistingChange.toString(Sm);
}

llvm::Error FileChanges::add(const SourceManager &Sm, const CodeChange &R) {
  // Check the file path.
  if (!Replaces.empty() && R.getFile() != Replaces.begin()->getFile())
    return llvm::make_error<CodeChangeError>(
        CodeChangeError::wrong_file_path);

  // We know that there currently no overlapping replacements.
  
  if(Replaces.empty()) {
    Replaces.insert(R);
    return llvm::Error::success();
  } 

  // Find the first entry that is not lower that R.
  auto I = Replaces.lower_bound(R);
  auto Hint = I;

  if(I == Replaces.end()) {
    assert(I != Replaces.begin());
    // If we are at the end, it means that all other changes compare
    // lower than us.
    // the only case where order is really important is when
    // we have two inserts at the same position.
    // but in that case such change would compare equivalent to R.
    // and we wouldn't be taking this branch
    --I;
  }
  
  auto M = R.isMergeable(*I);
  if(M == CodeChange::NONOVERLAP) {
    Replaces.insert(Hint, R);
    return llvm::Error::success();
  } else if(M == CodeChange::MERGE) {
    auto NewR = mergeChanges(R, *I);
    Hint = Replaces.erase(I);
    Replaces.insert(Hint, std::move(NewR));
    return llvm::Error::success();
  }
  else {
    //error
    return llvm::make_error<CodeChangeError>(
      CodeChangeError::overlap_conflict, Sm, *I, R);    
  }
}


void FileChanges::applyAll(Rewriter &Rewrite) const {

  if(Replaces.empty())
    return;

  auto ID = Replaces.begin()->getFile();
  SourceLocation File = Rewrite.getSourceMgr().getLocForStartOfFile(ID);

  for (auto I = Replaces.rbegin(), E = Replaces.rend(); I != E; ++I) {

    auto Start = File.getLocWithOffset(I->getOffset());
    // ReplaceText returns false on success.
    // ReplaceText only fails if the source location is not a file location, in
    // which case we already returned false earlier.
    bool Fail = Rewrite.ReplaceText(Start, I->getLength(), I->getReplacementText());
    if(Fail)
      assert(false);
  }
}

llvm::Error TUChanges::add(const SourceManager &Sm, const CodeChange &R) {
  if(R.isValid()) {
    return Replaces[R.getFile()].add(Sm, R);
  }
  else
    return llvm::make_error<ReplacementError>(
      replacement_error::wrong_file_path); 
}


bool overwriteChangedFiles(ASTContext &Context,
  const FileChanges &Changes, StringRef ToolName) {

  if(!Changes.empty()) {
    Rewriter Rewrite(Context.getSourceManager(), Context.getLangOpts());

    Changes.applyAll(Rewrite);
    if (Rewrite.overwriteChangedFiles()) {
      const FileEntry *F = Context.getSourceManager().getFileEntryForID(
        Changes.begin()->getFile());
      StringRef FileName = F ? F->getName() : "<unknown>";

      llvm::errs() << ToolName << " failed to apply suggested fixes to file " 
      << FileName << "\n";
      return false;
    } else {
      llvm::errs() << ToolName << " applied suggested fixes.\n";
    }
  }
  return true;
}


bool overwriteChangedFiles(ASTContext &Context,
  const std::map<FileID, FileChanges> &Changes, StringRef ToolName) {

  bool Result = true;
  for (auto &FileAndChanges : Changes) {
    Result = overwriteChangedFiles(Context, FileAndChanges.second, ToolName) && Result;
  }
  return Result;
}

bool overwriteChangedFiles(clang::ASTContext &Context,
  const TUChanges &Changes, llvm::StringRef ToolName) {

  bool Result = true;
  for (auto &FileCh : Changes) {
    Result = overwriteChangedFiles(Context, FileCh.second, ToolName) && Result;
  }
  return Result;
}


void overwriteChangedFiles(ASTContext &Context, const Replacements &Replaces, StringRef Name) {

  if(!Replaces.empty()) {
    Rewriter Rewrite(Context.getSourceManager(), Context.getLangOpts());

    if (!applyAllReplacements(Replaces, Rewrite)) {
      llvm::errs() << "Can't apply replacements for file " 
      << Replaces.begin()->getFilePath() << "\n";
    }
    if (Rewrite.overwriteChangedFiles()) {
      llvm::errs() << Name << " failed to apply suggested fixes to file " 
      << Replaces.begin()->getFilePath() << "\n";
    } else {
      llvm::errs() << Name << " applied suggested fixes.\n";
    }
  }
}

void overwriteChangedFiles(ASTContext &Context, const StringMap<Replacements> &FileReplacements, StringRef Name) {

  if(!FileReplacements.empty()) {
    for (const auto &FileAndReplacements : FileReplacements) {
      overwriteChangedFiles(Context, FileAndReplacements.second, Name);
    }
  }
}


} // namespace nodecpp
