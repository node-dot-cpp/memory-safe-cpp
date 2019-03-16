#!/bin/sh

cp build/release/bin/nodecpp-* /usr/local/bin

mkdir -p /usr/local/lib/clang/7.1.0/include/
cp -r build/release/lib/clang/7.1.0/include/* /usr/local/lib/clang/7.1.0/include/



