#!/bin/sh
set -ev

rm -Rf build/travis
mkdir -p build/travis
cd build/travis

cmake -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_CHECKER_SOURCE_DIR=${PWD}/../.. -G Ninja ../../3rdparty/llvm

cmake --build . --target check-safememory-tools

cd ../..

