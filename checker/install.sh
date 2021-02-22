#!/bin/sh

cp build/release/bin/safememory-* /usr/local/bin

mkdir -p /usr/local/lib/clang/9.0.1/include/
cp -r build/release/lib/clang/9.0.1/include/* /usr/local/lib/clang/9.0.1/include/
