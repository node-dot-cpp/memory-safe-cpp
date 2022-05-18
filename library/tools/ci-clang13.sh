#!/bin/sh
set -ev

rm -Rf library/build/clang13
mkdir -p library/build/clang13
cd library/build/clang13

export CC=clang-13
export CXX=clang++-13

cmake -DCMAKE_BUILD_TYPE=Release -DSAFEMEMORY_TEST=ON -G "Unix Makefiles" ../..

cmake --build .

ctest --output-on-failure

