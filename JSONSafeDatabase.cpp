//===--- JSONSafeDatabase.cpp - ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains the implementation of the JSONSafeDatabase.
//
//===----------------------------------------------------------------------===//

#include "JSONSafeDatabase.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/CompilationDatabasePluginRegistry.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/StringSaver.h"
#include <system_error>
#include <sstream>

namespace clang {
namespace tidy {

std::unique_ptr<JSONSafeDatabase>
JSONSafeDatabase::loadFromDirectory(StringRef BuildDirectory,
                                       std::string &ErrorMessage) {
  llvm::raw_string_ostream ErrorStream(ErrorMessage);
  std::string DatabaseErrorMessage;
  if (std::unique_ptr<JSONSafeDatabase> DB =
          loadFromDirectory2(BuildDirectory, DatabaseErrorMessage))
    return DB;
  ErrorStream << "json-safe-library-database" << ": " << DatabaseErrorMessage << "\n";
  return nullptr;
}

static std::unique_ptr<JSONSafeDatabase>
findCompilationDatabaseFromDirectory(StringRef Directory,
                                     std::string &ErrorMessage) {
  std::stringstream ErrorStream;
  bool HasErrorMessage = false;
  while (!Directory.empty()) {
    std::string LoadErrorMessage;

    if (std::unique_ptr<JSONSafeDatabase> DB =
            JSONSafeDatabase::loadFromDirectory(Directory, LoadErrorMessage))
      return DB;

    if (!HasErrorMessage) {
      ErrorStream << "No compilation database found in " << Directory.str()
                  << " or any parent directory\n" << LoadErrorMessage;
      HasErrorMessage = true;
    }

    Directory = llvm::sys::path::parent_path(Directory);
  }
  ErrorMessage = ErrorStream.str();
  return nullptr;
}

std::unique_ptr<JSONSafeDatabase>
JSONSafeDatabase::autoDetectFromSource(StringRef SourceFile,
                                          std::string &ErrorMessage) {
  SmallString<1024> AbsolutePath(clang::tooling::getAbsolutePath(SourceFile));
  StringRef Directory = llvm::sys::path::parent_path(AbsolutePath);

  std::unique_ptr<JSONSafeDatabase> DB =
      findCompilationDatabaseFromDirectory(Directory, ErrorMessage);

  if (!DB)
    ErrorMessage = ("Could not auto-detect compilation database for file \"" +
                   SourceFile + "\"\n" + ErrorMessage).str();
  return DB;
}

std::unique_ptr<JSONSafeDatabase>
JSONSafeDatabase::autoDetectFromDirectory(StringRef SourceDir,
                                             std::string &ErrorMessage) {
  SmallString<1024> AbsolutePath(clang::tooling::getAbsolutePath(SourceDir));

  std::unique_ptr<JSONSafeDatabase> DB =
      findCompilationDatabaseFromDirectory(AbsolutePath, ErrorMessage);

  if (!DB)
    ErrorMessage = ("Could not auto-detect compilation database from directory \"" +
                   SourceDir + "\"\n" + ErrorMessage).str();
  return DB;
}


std::unique_ptr<JSONSafeDatabase>
JSONSafeDatabase::loadFromDirectory2(StringRef Directory, std::string &ErrorMessage) {
  SmallString<1024> JSONDatabasePath(Directory);
  llvm::sys::path::append(JSONDatabasePath, "safe_library.json");
  return loadFromFile(
      JSONDatabasePath, ErrorMessage);
}

std::unique_ptr<JSONSafeDatabase>
JSONSafeDatabase::loadFromFile(StringRef FilePath,
                                      std::string &ErrorMessage) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> DatabaseBuffer =
      llvm::MemoryBuffer::getFile(FilePath);
  if (std::error_code Result = DatabaseBuffer.getError()) {
    ErrorMessage = "Error while opening JSON database: " + Result.message();
    return nullptr;
  }
  std::unique_ptr<JSONSafeDatabase> Database(
      new JSONSafeDatabase(std::move(*DatabaseBuffer)));
  if (!Database->parse(ErrorMessage))
    return nullptr;
  return Database;
}

std::unique_ptr<JSONSafeDatabase>
JSONSafeDatabase::loadFromBuffer(StringRef DatabaseString,
                                        std::string &ErrorMessage) {
  std::unique_ptr<llvm::MemoryBuffer> DatabaseBuffer(
      llvm::MemoryBuffer::getMemBuffer(DatabaseString));
  std::unique_ptr<JSONSafeDatabase> Database(
      new JSONSafeDatabase(std::move(DatabaseBuffer)));
  if (!Database->parse(ErrorMessage))
    return nullptr;
  return Database;
}


void JSONSafeDatabase::getValues(ArrayRef<llvm::yaml::ScalarNode*> Refs,
                   std::set<std::string> &Values) const {
  for (int I = 0, E = Refs.size(); I != E; ++I) {

    SmallString<128> ValueStorage;
    Values.emplace(Refs[I]->getValue(ValueStorage));
  }
}

bool JSONSafeDatabase::parse(std::string &ErrorMessage) {
  llvm::yaml::document_iterator I = YAMLStream.begin();
  if (I == YAMLStream.end()) {
    ErrorMessage = "Error while parsing YAML.";
    return false;
  }
  llvm::yaml::Node *Root = I->getRoot();
  if (!Root) {
    ErrorMessage = "Error while parsing YAML.";
    return false;
  }
  llvm::yaml::SequenceNode *Array = dyn_cast<llvm::yaml::SequenceNode>(Root);
  if (!Array) {
    ErrorMessage = "Expected array.";
    return false;
  }
  for (auto& NextObject : *Array) {
    llvm::yaml::MappingNode *Object = dyn_cast<llvm::yaml::MappingNode>(&NextObject);
    if (!Object) {
      ErrorMessage = "Expected object.";
      return false;
    }
    // llvm::yaml::ScalarNode *Directory = nullptr;
    // llvm::Optional<std::vector<llvm::yaml::ScalarNode *>> Command;
    // llvm::yaml::ScalarNode *File = nullptr;
    // llvm::yaml::ScalarNode *Output = nullptr;
    llvm::Optional<std::vector<llvm::yaml::ScalarNode *>> Functions;
    llvm::Optional<std::vector<llvm::yaml::ScalarNode *>> Types;
    for (auto& NextKeyValue : *Object) {
      llvm::yaml::ScalarNode *KeyString =
          dyn_cast<llvm::yaml::ScalarNode>(NextKeyValue.getKey());
      if (!KeyString) {
        ErrorMessage = "Expected strings as key.";
        return false;
      }
      SmallString<10> KeyStorage;
      StringRef KeyValue = KeyString->getValue(KeyStorage);
      llvm::yaml::Node *Value = NextKeyValue.getValue();
      if (!Value) {
        ErrorMessage = "Expected value.";
        return false;
      }
      // llvm::yaml::ScalarNode *ValueString =
      //     dyn_cast<llvm::yaml::ScalarNode>(Value);
      llvm::yaml::SequenceNode *SequenceString =
          dyn_cast<llvm::yaml::SequenceNode>(Value);
      // if (KeyValue == "arguments" && !SequenceString) {
      //   ErrorMessage = "Expected sequence as value.";
      //   return false;
      // } else if (KeyValue != "arguments" && !ValueString) {
      //   ErrorMessage = "Expected string as value.";
      //   return false;
      // }
      if (!SequenceString) {
        ErrorMessage = "Expected sequence as value.";
        return false;
      }
      // if (KeyValue == "directory") {
      //   Directory = ValueString;
      // } else if (KeyValue == "arguments") {
      //   Command = std::vector<llvm::yaml::ScalarNode *>();
      //   for (auto &Argument : *SequenceString) {
      //     auto Scalar = dyn_cast<llvm::yaml::ScalarNode>(&Argument);
      //     if (!Scalar) {
      //       ErrorMessage = "Only strings are allowed in 'arguments'.";
      //       return false;
      //     }
      //     Command->push_back(Scalar);
      //   }
      // } else if (KeyValue == "command") {
      //   if (!Command)
      //     Command = std::vector<llvm::yaml::ScalarNode *>(1, ValueString);
      // } else if (KeyValue == "file") {
      //   File = ValueString;
      // } else if (KeyValue == "output") {
      //   Output = ValueString;
      // } else 
      if (KeyValue == "functions") {
        Functions = std::vector<llvm::yaml::ScalarNode *>();
        for (auto &Argument : *SequenceString) {
          auto Scalar = dyn_cast<llvm::yaml::ScalarNode>(&Argument);
          if (!Scalar) {
            ErrorMessage = "Only strings are allowed in 'functions'.";
            return false;
          }
          Functions->push_back(Scalar);
        }
      } else if (KeyValue == "types") {
        Types = std::vector<llvm::yaml::ScalarNode *>();
        for (auto &Argument : *SequenceString) {
          auto Scalar = dyn_cast<llvm::yaml::ScalarNode>(&Argument);
          if (!Scalar) {
            ErrorMessage = "Only strings are allowed in 'types'.";
            return false;
          }
          Types->push_back(Scalar);
        }
      } else {
        ErrorMessage = ("Unknown key: \"" +
                        KeyString->getRawValue() + "\"").str();
        return false;
      }
    }
    // if (!File) {
    //   ErrorMessage = "Missing key: \"file\".";
    //   return false;
    // }
    // if (!Command) {
    //   ErrorMessage = "Missing key: \"command\" or \"arguments\".";
    //   return false;
    // }
    // if (!Directory) {
    //   ErrorMessage = "Missing key: \"directory\".";
    //   return false;
    // }
    // SmallString<8> FileStorage;
    // StringRef FileName = File->getValue(FileStorage);
    // SmallString<128> NativeFilePath;
    // if (llvm::sys::path::is_relative(FileName)) {
    //   SmallString<8> DirectoryStorage;
    //   SmallString<128> AbsolutePath(
    //       Directory->getValue(DirectoryStorage));
    //   llvm::sys::path::append(AbsolutePath, FileName);
    //   llvm::sys::path::native(AbsolutePath, NativeFilePath);
    // } else {
    //   llvm::sys::path::native(FileName, NativeFilePath);
    // }
    // auto Cmd = CompileCommandRef(Directory, File, *Command, Output);
    // IndexByFile[NativeFilePath].push_back(Cmd);
    // AllCommands.push_back(Cmd);
    // MatchTrie.insert(NativeFilePath);

    if(Functions) {
      AllFunctions.insert(AllFunctions.end(), Functions->begin(), Functions->end());
    }
    if(Types) {
      AllTypes.insert(AllTypes.end(), Types->begin(), Types->end());
    }
  }
  return true;
}

} // end namespace tidy
} // end namespace clang
