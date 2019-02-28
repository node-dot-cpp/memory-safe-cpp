

Windows build
=================================

Requirements
------------

1. git command line client 
2. Visual Studio 2017 (C++ command line tools)
3. Cmake (I have 3.13.2, but any recent version should work) https://cmake.org/
4. Python 2.7 ( Version 3.x will not work, must be 2.7.x) https://www.python.org/downloads/release/python-2715/

All git, Cmake and Python 2.7 must be in your `PATH` environment so scripts can call them.

Simple build (Release build with VS2017 command line tools, using msbuild)
--------------------------------------------------------------------------

Checkout `node-dot-cpp/memory-safe-cpp`, if you haven't done it already.
Open a console from 'x64 Native Tools Command Prompt for VS 2017' (from Windows start menu, under Visual Studio 2017 folder) and go to the recently checked out folder `memory-safe-cpp/checker`

First run `checkout.bat` script, it will clone all llvm/clang dependencies in their required locations.
Then run `build.bat` script, it will configure the build using `cmake` and will build the tools. A short automated test suite should run after the build is complete.


Linux build
===========

Requirements
------------

0. git client
1. GCC 7
2. Make
3. Cmake (I have 3.10.2, but any recent version should work) https://cmake.org/
4. Python 2.7 ( Version 3.x will not work, must be 2.7.x) 


Simple build (Release build using Make)
---------------------------------------

Checkout `node-dot-cpp/memory-safe-cpp`, if you haven't done it already and go to the recently checked out folder `memory-safe-cpp/checker`.

First run `checkout.sh` script, it will clone all llvm/clang dependencies in their required locations.
Then run `build.sh` script, it will configure the build using `cmake` and will build the tools. A short automated test suite should run after the build is complete.


Other build options
===================

Tools build is `cmake` based, and as such has lot of options and flexibility for developers.
I normally build using 'cmake / ninja / command line VC', ninja (https://ninja-build.org/) is a lot faster on incremental builds.

I have git and cmake in my system `PATH`. For MSCV and Python I use a bat file to set up environment variables. This will help if you have already installed Python 3 in your environment and don't want to break things by setting global `PATH` to Python 2.7. Also if you have more than one version of Visual Studio, etc.

I put all bat files at my project root that is `C:\node-dot-cpp\memory-safe-cpp\checker`, you can use any root folder but update scripts in this tutorial to match your setup.

I use `build-env.bat` with the following:

	title build

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%

	%ComSpec% /k ""C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


You will need to change paths to match your system.

Assuming you have ninja on your PATH

	checker> cmake-ninja-debug.bat
	checker\build\debug> ninja nodecpp-checker
	checker\build\debug> ninja nodecpp-safe-library
	checker\build\debug> ninja check-nodecpp-checker

	checker\build\debug> cd ..\..

All tools should end at `build\debug\bin` folder.

For testing I use a separate `test-debug-env.bat` file to set up environmnet to run the tool and tests.

	title test

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%
	set PATH=C:\node-dot-cpp\memory-safe-cpp\checker\3rdparty\llvm\utils\lit;%PATH%
	set PATH=C:\node-dot-cpp\memory-safe-cpp\checker\build\debug\bin;%PATH%


Then, to manually run automated tests:

	checker> llvm-lit.py test

To run the tool on a single test file and see the output,

	checker> nodecpp-checker test\nodecpp-checker\s1.cpp
	

Next
====
See file [CHECKER-QUICK-START.md](CHECKER-QUICK-START.md) for a very first, very quick sample use.


Or you can see [CHECKER-RUN.md](CHECKER-RUN.md) to set up the environment to run the tool over your own files or projects.

Or take a look at [CHECKER-TESTS.md](CHECKER-TESTS.md) to run or add automated test cases.


 
