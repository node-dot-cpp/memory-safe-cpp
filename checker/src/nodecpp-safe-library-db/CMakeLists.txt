set( LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  AllTargetsDescs
  AllTargetsInfos
  Support
  )

add_clang_executable(nodecpp-safe-library-db
  NodeCppSafeLibrary.cpp
  raiistdiofile.h
  )


target_link_libraries(nodecpp-safe-library-db
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  )

install(TARGETS nodecpp-safe-library-db
  RUNTIME DESTINATION bin)
