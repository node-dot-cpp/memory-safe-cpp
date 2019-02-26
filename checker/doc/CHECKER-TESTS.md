Running and adding automated tests
==================================

See file [CHECHER-QUICK-START.md](CHECHER-QUICK-START.md) for a very first, very quick sample use.

This document covers automated tests runned by `llvm-lit.py`. This is a llvm/clang automated testing tool. You can run automated tests only if you built the tools from source code, since the tool is designed to work that way.

If you look at `test.bat`, it is simple:

	set PATH=%cd%\3rdparty\llvm\utils\lit;%PATH%
	set PATH=%cd%\build\vs2017\Release\bin;%PATH%

	llvm-lit.py test

Sets the `PATH` and runs the tool over the `test` folder. The `test` folder has some special files `test/lit.cfg` and `test/lit.site.cfg.in` used to configure lit, and also `test/CMakeLists.txt` has some special content used by `cmake` during build configuration to properly set up the test environment.


Each `.cpp` in that folder is considered a test case. To add a new test case, simple add a new `.cpp` file under `test/nodecpp-checker` and `llvm-lit` will automatically pick it up. 

 You can also add a new folder under `test` to put test cases, in that case you may need to copy file `safe_library.json` and `compile_flags.txt`. Please see [CHECHER-RUN.md](CHECHER-RUN.md) to better understand how those files fit inside the process and when you may need to modify them for your specific needs. Folders with name `Inputs` are treated specially, test cases are not scanned under such folders. That is the place to put headers files and mocks that may be needed by tests.


Important is to notice that automated tests don't access the __std__ library on your system, or the real `safe_ptr.h` library, they use a small _mock_ of them found in `Inputs` folders. This is done such way to improve test stability across different platforms (`llvm/clang` tests do the same thing).


Each `.cpp` file has a header line, that tells the runner how to run this particular test. In our case, they are all the same:

	// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"



The `FileCheck` utility will try to match the std out of checker tool with the lines prefixed by `CHECK:` in the same source code. We also add `-implicit-check-not` for the tool to verify that the tool is not generating any more output than we are specifically expecting for this test file.

Then, for each warning the checker generates, we have a line in the source file that should match:

	// CHECK: :[[@LINE-1]]:2: warning: (S1.1)


For more information on `llvm-lit.py` pleasee see https://llvm.org/docs/CommandGuide/lit.html


Safe library and compilation database
-------------------------------------
Is important to notice is that `nodecpp-checker` will automatically pick `safe_library.json` and `compile_flags.txt` from the source folder.
Please see [CHECHER-RUN.md](CHECHER-RUN.md) to better understand how those files fit inside the process and when you may need to modify them for your specific needs.



