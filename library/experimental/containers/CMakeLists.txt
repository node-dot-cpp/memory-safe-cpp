
add_clang_executable(container-test
  main.cpp
  )

set_property(TARGET container-test PROPERTY CXX_STANDARD 17)

install(TARGETS nodecpp-instrument
  RUNTIME DESTINATION bin)
