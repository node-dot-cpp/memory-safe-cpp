#!/bin/sh
set -ev

rm -Rf build/release
mkdir -p build/release
cd build/release


# Uncomment lines below to use clang or change to other compiler
#
#export CC=clang-7
#export CXX=clang++-7

cmake -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_SAFEMEMORY_SOURCE_DIR=${PWD}/../.. -G "Unix Makefiles" ../../3rdparty/llvm-project/llvm

cmake --build . --target safememory-checker
cmake --build . --target safememory-instrument
cmake --build . --target safememory-library-db
cmake --build . --target safememory-odr

cmake --build . --target check-safememory-tools


cd ../..

