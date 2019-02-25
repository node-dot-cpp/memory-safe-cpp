#!/bin/sh

mkdir build
cd build
mkdir release
cd release

cmake -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_CHECKER_SOURCE_DIR=${PWD}/../.. -G "Unix Makefiles" ../../3rdparty/llvm

make nodecpp-checker
make nodecpp-safe-library

make check-nodecpp-checker

cd ../..

