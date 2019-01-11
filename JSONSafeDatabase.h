//===--- JSONSafeDatabase.h - ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  The JSONSafeDatabase finds safe types and functions databases supplied as 
//  a file 'safe_functions.json'.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLING_JSONSAFEDATABASE_H
#define LLVM_CLANG_TOOLING_JSONSAFEDATABASE_H

#include "clang/Basic/LLVM.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/FileMatchTrie.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"
#include <memory>
#include <string>
#include <vector>
#include <set>

namespace clang {
namespace tidy {

/// \brief A JSON based safe functions and types database.
///
/// JSON safe database files must contain a list of JSON objects which
/// provide the lists of safe types and functions from standard libraries:
/// [
///   { "functions": ["<aFunction>", "<otherFunction>"],
///     "types": ["<aType>", "<otherType>"]
///   },
///   { "functions": ["<aFunction>", "<otherFunction>"],
///     "types": ["<aType>", "<otherType>"]
///   },
///   ...
/// ]
///
class JSONSafeDatabase {
public:

  /// \brief Loads a compilation database from a build directory.
  ///
  /// Looks at the specified 'BuildDirectory' and creates a compilation database
  /// that allows to query compile commands for source files in the
  /// corresponding source tree.
  ///
  /// Returns NULL and sets ErrorMessage if we were not able to build up a
  /// compilation database for the build directory.
  ///
  /// FIXME: Currently only supports JSON compilation databases, which
  /// are named 'compile_commands.json' in the given directory. Extend this
  /// for other build types (like ninja build files).
  static std::unique_ptr<JSONSafeDatabase>
  loadFromDirectory(StringRef BuildDirectory, std::string &ErrorMessage);

  /// \brief Tries to detect a compilation database location and load it.
  ///
  /// Looks for a compilation database in all parent paths of file 'SourceFile'
  /// by calling loadFromDirectory.
  static std::unique_ptr<JSONSafeDatabase>
  autoDetectFromSource(StringRef SourceFile, std::string &ErrorMessage);

  /// \brief Tries to detect a compilation database location and load it.
  ///
  /// Looks for a compilation database in directory 'SourceDir' and all
  /// its parent paths by calling loadFromDirectory.
  static std::unique_ptr<JSONSafeDatabase>
  autoDetectFromDirectory(StringRef SourceDir, std::string &ErrorMessage);


  /// \brief Loads a JSON safe database from the specified folder.
  ///
  /// Returns NULL and sets ErrorMessage if the database could not be
  /// loaded from the given file.
  static std::unique_ptr<JSONSafeDatabase>
  loadFromDirectory2(StringRef Directory, std::string &ErrorMessage);


  /// \brief Loads a JSON safe database from the specified file.
  ///
  /// Returns NULL and sets ErrorMessage if the database could not be
  /// loaded from the given file.
  static std::unique_ptr<JSONSafeDatabase>
  loadFromFile(StringRef FilePath, std::string &ErrorMessage);

  /// \brief Loads a JSON safe database from a data buffer.
  ///
  /// Returns NULL and sets ErrorMessage if the database could not be loaded.
  static std::unique_ptr<JSONSafeDatabase>
  loadFromBuffer(StringRef DatabaseString, std::string &ErrorMessage);

  /// \brief Returns the set of safe types names.
  void getTypes(std::set<std::string>& Types) const {
    getValues(AllTypes, Types);
  }
  
  /// \brief Returns the set of safe functions names.
  void getFunctions(std::set<std::string>& Functions) const {
    getValues(AllFunctions, Functions);
  }


private:
  /// \brief Constructs a JSON safe database on a memory buffer.
  JSONSafeDatabase(std::unique_ptr<llvm::MemoryBuffer> Database)
      : Database(std::move(Database)),
        YAMLStream(this->Database->getBuffer(), SM) {}


  void getValues(ArrayRef<llvm::yaml::ScalarNode*> Refs,
                   std::set<std::string> &Values) const;
                   
  /// \brief Parses the database file and creates the index.
  ///
  /// Returns whether parsing succeeded. Sets ErrorMessage if parsing
  /// failed.
  bool parse(std::string &ErrorMessage);


  /// All the compile commands in the order that they were provided in the
  /// JSON stream.
  std::vector<llvm::yaml::ScalarNode*> AllFunctions;
  std::vector<llvm::yaml::ScalarNode*> AllTypes;

  std::unique_ptr<llvm::MemoryBuffer> Database;
  llvm::SourceMgr SM;
  llvm::yaml::Stream YAMLStream;
};

} // end namespace tidy
} // end namespace clang

#endif
