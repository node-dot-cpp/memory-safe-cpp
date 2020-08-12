#!/bin/sh
set -ev

cd 3rdparty

rm -Rf llvm
rm -Rf clang
rm -Rf clang-tools-extra

git clone --depth 1 -b release_90 https://github.com/llvm-mirror/llvm.git
git clone --depth 1 -b release_90 https://github.com/llvm-mirror/clang.git

cd clang
git apply ../clang_release_90.diff

cd ../..
