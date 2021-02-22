set( LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  Support
  )

add_clang_executable(safememory-instrument
  CodeChange.cpp
  Dezombiefy.cpp
  InclusionRewriter.cpp
  NodeCppDezombify.cpp
  SequenceCheckAndFix.cpp
  )


target_link_libraries(safememory-instrument
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  clangRewriteFrontend
  )

install(TARGETS safememory-instrument
  RUNTIME DESTINATION bin)
