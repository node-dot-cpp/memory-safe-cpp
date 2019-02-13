

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


I put all bat files at my project root that is `C:\node-dot-cpp\static-checker`, you can use any root folder but update scripts in this tutorial to match your setup.

I use `build-env.bat` with the following:

	title build

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%

	%ComSpec% /k ""D:\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


You will need to change paths to match your system.


Build
-----

When you double click on `build-env.bat` you will get a console, with python and VC command line environmnet.

First clone and checkout everything,

	static-checker> git clone -b release_70 https://github.com/llvm-mirror/llvm.git
	static-checker> cd llvm\tools
	static-checker\llvm\tools> git clone -b node-dot-cpp_70 https://github.com/node-dot-cpp/clang.git
	static-checker\llvm\tools> cd clang\tools
	static-checker\llvm\tools\clang\tools> git clone -b release_70 https://github.com/llvm-mirror/clang-tools-extra.git extra
	static-checker\llvm\tools\clang\tools> git clone https://github.com/node-dot-cpp/clang-tools-nodecpp.git nodecpp
	static-checker\llvm\tools\clang\tools> cd ..\..\..\..


Now build (release with nmake):

	static-checker> md build\release
	static-checker> cd build\release

	static-checker\build\release> cmake -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" ..\..\llvm

	static-checker\build\release> nmake nodecpp-checker

	static-checker\build\release> nmake nodecpp-safe-library

	static-checker\build\release> nmake FileCheck

	static-checker\build\release> cd ..\..

All tools should end at `build\release\bin` folder, `FileCheck` is a llvm/clang tool used in automated testing.
You can also build with `ninja` or generate Visual Studio project files, or any of the common `cmake` options. 


Test
----

I use a separate `test-env.bat` file to set up environmnet to run the tool and tests.

	title test

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%
	set PATH=C:\node-dot-cpp\static-checker\llvm\utils\lit;%PATH%
	set PATH=C:\node-dot-cpp\static-checker\build\release\bin;%PATH%

	%ComSpec% /k ""D:\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


Then, to run automated tests just use `run.bat` script

	static-checker> run.bat

If you are really lucky, will see a bunch of test files, with the 'PASS' legend on the left :)

To run our tool on a single test file and see the output,

	static-checker> nodecpp-checker llvm\tools\clang\tools\nodecpp-checker\test\nodecpp-checker\s1.cpp --
	
Don't forget the double hypen (`--`) at the end.


Please see file [CHECHER-RUN.md](CHECHER-RUN.md) to set up the environment to run the tool over your own files or projects.

