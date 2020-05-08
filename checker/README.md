# static-checker
Tools for static check of C++ following rules of [__memory-safe-cpp__](https://github.com/node-dot-cpp/memory-safe-cpp)


Goals
-----

The memory-safe C++ (as defined at memory-safe-cpp) require of static checks and runtime checks. This project is intended to provide the tools for the static checks.


Overview
--------
The static check tools are based on __clang7 tooling AST__. This means that any source code that is accepted by clang7 _should_ be accepted by the tool. It also means that building the tool implies building big part of clang7.


Please see [doc/CHECKER-QUICK-START.md](doc/CHECKER-QUICK-START.md) for a quick introduction to build and use of the tools.

