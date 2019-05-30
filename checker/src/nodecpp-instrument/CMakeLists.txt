set( LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  AllTargetsDescs
  AllTargetsInfos
  Support
  )

add_clang_executable(nodecpp-instrument
  NodeCppDezombify.cpp
  ExpandUserIncludesAction.cpp
  InclusionRewriter.cpp
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
