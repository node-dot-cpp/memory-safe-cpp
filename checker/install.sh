#!/bin/sh

cp build/release/bin/nodecpp-* /usr/local/bin

mkdir --parents /usr/local/lib/clang/7.1.0/include/
cp --recursive build/release/lib/clang/7.1.0/include/* /usr/local/lib/clang/7.1.0/include/



