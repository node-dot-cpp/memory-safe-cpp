//===--- ClangTidyModule.h - clang-tidy -------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef NODECPP_CHECKER_CLANGTIDYMODULE_H
#define NODECPP_CHECKER_CLANGTIDYMODULE_H

#include "ClangTidy.h"
#include "llvm/ADT/StringRef.h"
#include <functional>
#include <map>
#include <string>
#include <utility>

namespace nodecpp {
namespace checker {

/// \brief A collection of \c ClangTidyCheckFactory instances.
///
/// All clang-tidy modules register their check factories with an instance of
/// this object.
class ClangTidyCheckFactories {
public:
  typedef std::function<ClangTidyCheck *(StringRef Name,
                                         ClangTidyContext *Context)>
      CheckFactory;

  /// \brief Registers check \p Factory with name \p Name.
  ///
  /// For all checks that have default constructors, use \c registerCheck.
  void registerCheckFactory(StringRef Name, CheckFactory Factory);

  /// \brief Registers the \c CheckType with the name \p Name.
  ///
  /// This method should be used for all \c ClangTidyChecks that don't require
  /// constructor parameters.
  ///
  /// For example, if have a clang-tidy check like:
  /// \code
  /// class MyTidyCheck : public ClangTidyCheck {
  ///   void registerMatchers(ast_matchers::MatchFinder *Finder) override {
  ///     ..
  ///   }
  /// };
  /// \endcode
  /// you can register it with:
  /// \code
  /// class MyModule : public ClangTidyModule {
  ///   void addCheckFactories(ClangTidyCheckFactories &Factories) override {
  ///     Factories.registerCheck<MyTidyCheck>("myproject-my-check");
  ///   }
  /// };
  /// \endcode
  template <typename CheckType> void registerCheck(StringRef CheckName) {
    registerCheckFactory(CheckName,
                         [](StringRef Name, ClangTidyContext *Context) {
                           return new CheckType(Name, Context);
                         });
  }

  /// \brief Create instances of all checks matching \p CheckRegexString and
  /// store them in \p Checks.
  ///
  /// The caller takes ownership of the return \c ClangTidyChecks.
  void createChecks(ClangTidyContext *Context,
                    std::vector<std::unique_ptr<ClangTidyCheck>> &Checks);

  typedef std::map<std::string, CheckFactory> FactoryMap;
  FactoryMap::const_iterator begin() const { return Factories.begin(); }
  FactoryMap::const_iterator end() const { return Factories.end(); }
  bool empty() const { return Factories.empty(); }

private:
  FactoryMap Factories;
};

/// \brief A clang-tidy module groups a number of \c ClangTidyChecks and gives
/// them a prefixed name.
class ClangTidyModule {
public:
  virtual ~ClangTidyModule() {}

  /// \brief Implement this function in order to register all \c CheckFactories
  /// belonging to this module.
  virtual void addCheckFactories(ClangTidyCheckFactories &CheckFactories) = 0;

  /// \brief Gets default options for checks defined in this module.
  virtual ClangTidyOptions getModuleOptions();
};

} // end namespace checker
} // end namespace nodecpp

#endif // NODECPP_CHECKER_CLANGTIDYMODULE_H
