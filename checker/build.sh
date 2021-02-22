#!/bin/sh
set -ev

rm -Rf build/release
mkdir -p build/release
cd build/release


# Uncomment lines below to use clang or change to other compiler
#
#export CC=clang-7
#export CXX=clang++-7

cmake -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_CHECKER_SOURCE_DIR=${PWD}/../.. -G "Unix Makefiles" ../../3rdparty/llvm

make --jobs=3 safememory-checker
make --jobs=3 safememory-instrument
make --jobs=3 safememory-library-db
make --jobs=3 safememory-odr

make --jobs=3 check-safememory-tools

cd ../..

