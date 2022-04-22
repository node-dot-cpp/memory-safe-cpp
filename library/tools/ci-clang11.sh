#!/bin/sh
set -ev

rm -Rf library/build/clang11
mkdir -p library/build/clang11
cd library/build/clang11

export CC=clang-11
export CXX=clang++-11

cmake -DCMAKE_BUILD_TYPE=Release -DSAFEMEMORY_TEST=ON -G "Unix Makefiles" ../..

cmake --build .

ctest --output-on-failure

