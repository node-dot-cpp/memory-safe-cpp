#!/bin/sh

mkdir build
cd build
mkdir release
cd release


# Uncomment lines below to use clang or change to other compiler
#
#export CC=clang-7
#export CXX=clang++-7

cmake -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_CHECKER_SOURCE_DIR=${PWD}/../.. -G "Unix Makefiles" ../../3rdparty/llvm

make nodecpp-checker
make nodecpp-safe-library

make check-nodecpp-checker

cd ../..

