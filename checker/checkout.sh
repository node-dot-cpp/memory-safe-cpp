#!/bin/sh
set -ev

cd 3rdparty

rm -Rf llvm
rm -Rf clang
rm -Rf clang-tools-extra
rm -Rf llvm-project

git clone --depth 1 --branch llvmorg-11.1.0 https://github.com/llvm/llvm-project.git

cd llvm-project
git apply ../llvm-project_llvmorg-11.1.0.diff

cd ../..
