#!/bin/sh

PATH=$PWD/3rdparty/llvm-project/llvm/utils/lit:$PATH
PATH=$PWD/build/release/bin:$PATH

llvm-lit test
