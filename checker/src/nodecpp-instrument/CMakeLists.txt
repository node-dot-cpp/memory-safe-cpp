set( LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  Support
  )

add_clang_executable(nodecpp-instrument
  CodeChange.cpp
  Dezombiefy.cpp
  InclusionRewriter.cpp
  NodeCppDezombify.cpp
  SequenceCheckAndFix.cpp
  )


target_link_libraries(nodecpp-instrument
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  clangRewriteFrontend
  )

install(TARGETS nodecpp-instrument
  RUNTIME DESTINATION bin)
