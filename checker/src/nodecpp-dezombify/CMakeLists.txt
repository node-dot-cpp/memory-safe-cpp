set( LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  AllTargetsDescs
  AllTargetsInfos
  Support
  )

add_clang_executable(nodecpp-dezombify
  NodeCppDezombify.cpp
  )


target_link_libraries(nodecpp-dezombify
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  )

install(TARGETS nodecpp-dezombify
  RUNTIME DESTINATION bin)
