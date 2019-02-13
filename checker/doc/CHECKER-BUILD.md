

Building checker tools (Windows)
=================================

General description
-------------------
I normally build using 'cmake / ninja / command line VC', but this tutorial uses `msbuild` for simplicity.

Requirements
------------

0. some git client
1. Visual Studio 2017 (C++ command line tools)
2. Cmake (I have 3.13.2, but any recent version should work) https://cmake.org/
3. Python 2.7 ( Version 3.x will not work, must be 2.7.x) https://www.python.org/downloads/release/python-2715/

I have git and cmake in my system PATH. For MSCV and Python I use a bat file to set up environment variables. This is better because it allows to have more than one version installed and have scripts to set environment for each version. Also will help if you have already installed Python 3 in your environment and don't want to break things by setting global PATH to Python 2.7


I put all bat files at my project root that is `C:\node-dot-cpp\memory-safe-cpp\checker`, you can use any root folder but update scripts in this tutorial to match your setup.

I use `build-env.bat` with the following:

	title build

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%

	%ComSpec% /k ""D:\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


You will need to change paths to match your system.


Build (Release under VS2017)
----------------------------

When you double click on `build-env.bat` you will get a console, with python and VC command line environmnet.

First clone and checkout everything, you can use `checkout.bat` script.
Then for release build under Visual Studio 2017, you can use `build.bat` script.
Finally, to run the tests, you can use `test.bat`.

All three scripts are very small and simple, and can be used as a reference.

Build (Debug using ninja)
-------------------------

Assuming you have ninja on your PATH

	checker> cmake-ninja-debug.bat
	checker\build\debug> ninja nodecpp-checker
	checker\build\debug> ninja nodecpp-safe-library
	checker\build\debug> ninja FileCheck

	checker\build\debug> cd ..\..

All tools should end at `build\debug\bin` folder, `FileCheck` is a llvm/clang tool used in automated testing.

Test
----

I use a separate `test-env.bat` file to set up environmnet to run the tool and tests.

	title test

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%
	set PATH=C:\node-dot-cpp\memory-safe-cpp\checker\3rdparty\llvm\utils\lit;%PATH%
	set PATH=C:\node-dot-cpp\memory-safe-cpp\checker\build\debug\bin;%PATH%

	%ComSpec% /k ""D:\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


Then, to run automated tests just use `run.bat` script

	checker> llvm-lit.py test\nodecpp-checker

If you are really lucky, will see a bunch of test files, with the 'PASS' legend on the left :)

To run our tool on a single test file and see the output,

	checker> nodecpp-checker test\nodecpp-checker\s1.cpp --
	
Don't forget the double hypen (`--`) at the end. Some tests need extra arguments to run, please see the first line of each test for a reference.


Please see file [CHECHER-RUN.md](CHECHER-RUN.md) to set up the environment to run the tool over your own files or projects.

