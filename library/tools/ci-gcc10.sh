#!/bin/sh
set -ev

rm -Rf library/build/gcc10
mkdir -p library/build/gcc10
cd library/build/gcc10

export CC=gcc-10
export CXX=g++-10

cmake -DCMAKE_BUILD_TYPE=Release -DSAFEMEMORY_TEST=ON -G "Unix Makefiles" ../..

cmake --build .

ctest --output-on-failure

