#!/bin/sh

cd 3rdparty

git clone --depth 10 -b release_70 https://github.com/llvm-mirror/llvm.git
git clone --depth 10 -b release_70 https://github.com/llvm-mirror/clang.git
git clone --depth 10 -b release_70 https://github.com/llvm-mirror/clang-tools-extra.git

cd clang
git apply ../clang_release_70.diff

cd ../..

mkdir --parents include/clang/7/include
cp --recursive 3rdparty/clang/lib/Headers/* include/clang/7/include/
