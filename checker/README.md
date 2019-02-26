# static-checker
Tools for static check of C++ following rules of [__memory-safe-cpp__](https://github.com/node-dot-cpp/memory-safe-cpp)



Goals
-----

The memory-safe C++ (as defined at memory-safe-cpp) require of static checks and runtime checks. This project is intended to provide the tools for the static checks.


Overview
--------
The static check tools are based on __clang7 tooling AST__. This means that any source code that is accepted by clang7 _should_ be accepted by the tool. It also means that building the tool implies building big part of clang7.



Build (Windows)
---------------

Requirements:

1. git
2. Cmake
3. Python 2.7 (Python 3 will not work)
4. Visual Studio 2017 (C++)

Building clang based tools works better when source code is placed inside the llvm/clang source tree.
Because of that, this repository has no code, but acts as the root place for checkout of llvm/clang source tree, and then checkout the repository with the tool code inside that.

The easy way is to checkout this project and then using `checkout.bat`, `build.bat` and `test.bat` scripts. But this require your PATH environment can access git, cmake, python 2.7 and Visual Studio Command Line. What I find the most convenient is to create a file `build-env.bat` that set ups the environment, and put it in this root folder (git will ignore it). Then double click on `build-env.bat` will pop up a console with the environment correctly set. Below is a sample, please update paths to reflect your system and needs.

	title build

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%

	%ComSpec% /k ""C:\Program Files(x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


For a more detailed description please see [doc/CHECKER-BUILD.md](doc/CHECKER-BUILD.md)
