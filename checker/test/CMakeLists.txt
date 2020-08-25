# Test runner infrastructure for Clang-based tools. This configures the Clang
# test trees for use by Lit, and delegates to LLVM's lit test handlers.
#
# Note that currently we don't support stand-alone builds of Clang, you must
# be building Clang from within a combined LLVM+Clang checkout..

set(NODECPP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(NODECPP_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/..")

if (CMAKE_CFG_INTDIR STREQUAL ".")
  set(LLVM_BUILD_MODE ".")
else ()
  set(LLVM_BUILD_MODE "%(build_mode)s")
endif ()

string(REPLACE ${CMAKE_CFG_INTDIR} ${LLVM_BUILD_MODE} NODECPP_TOOLS_DIR ${LLVM_RUNTIME_OUTPUT_INTDIR})

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  )

option(NODECPP_TEST_USE_VG "Run Clang tools' tests under Valgrind" OFF)
if(NODECPP_TEST_USE_VG)
  set(NODECPP_TEST_EXTRA_ARGS ${CLANG_TEST_EXTRA_ARGS} "--vg")
endif()


set(NODECPP_TEST_DEPS
  nodecpp-checker
  nodecpp-instrument
  nodecpp-safe-library
  FileCheck
  not
)

add_lit_testsuite(check-nodecpp-tools "Running the node-dot-cpp tools regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${NODECPP_TEST_DEPS}
  ARGS ${NODECPP_TEST_EXTRA_ARGS}
  )

set_target_properties(check-nodecpp-tools PROPERTIES FOLDER "node-dot-cpp tools tests")
