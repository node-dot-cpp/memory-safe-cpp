set( LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  Support
  )

add_clang_executable(safe-memory-odr
  SafeMemoryOdr.cpp
  raiistdiofile.h
  )


target_link_libraries(safe-memory-odr
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  )

install(TARGETS safe-memory-odr
  RUNTIME DESTINATION bin)
