#!/bin/sh

cd 3rdparty

rm -Rf llvm
rm -Rf clang
rm -Rf clang-tools-extra

git clone --depth 10 -b release_70 https://github.com/llvm-mirror/llvm.git
git clone --depth 10 -b release_70 https://github.com/llvm-mirror/clang.git
git clone --depth 10 -b release_70 https://github.com/llvm-mirror/clang-tools-extra.git

cd clang
git apply ../clang_release_70.diff

cd ../..
