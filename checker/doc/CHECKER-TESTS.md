

Running and adding automated tests
==================================

We will assume that `nodecpp-checker` and `nodecpp-safe-library` tools are already built and in the PATH.
If not, take a look at [CHECKER-BUILD.md](CHECKER-BUILD.md).


Automated tests are runned by `llvm-lit.py`, this is a llvm/clang automated testing tool. You can run automated tests only if you built the tools from source code, since the tool is designed to work that way.

If you look at `test.bat`, it is simple:

	set PATH=%cd%\3rdparty\llvm\utils\lit;%PATH%
	set PATH=%cd%\build\vs2017\Release\bin;%PATH%

	llvm-lit.py test\nodecpp-checker

You have to set the `PATH` and run the tool over the `test/nodecpp-checker` folder.
Each `.cpp` in that folder is considered a test case. To add a new test case, simple add a new `.cpp` file and `llvm-lit` will automatically pick it up.


Each file has a header line, that tells the runner how to run this particular test. In our case, they are all the same:

	// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs | FileCheck %s -implicit-check-not="{{warning|error}}:"


Important is to notice that automated test cases don't use std includes, this is important to ensure test stability across different platforms (`llvm/clang` tests do the same thing). Instead tests use mock implementations of std library needed, those mocks can be found at `Input` folder.
The `FileCheck` utility will try to match the std out of checker tool with the lines prefixed by `CHECK:` in the same source code. We also add `-implicit-check-not` for the tool to verify that the tool is not generating any more output than we are specifically expecting for this test file.

Then, for each warning the checker generates, we have a line in the source file that should match:

	// CHECK: :[[@LINE-1]]:2: warning: (S1.1)


For more information on `llvm-lit.py` pleasee see https://llvm.org/docs/CommandGuide/lit.html


