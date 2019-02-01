

Building checker tools (Windows)
=================================

General description
-------------------
I use 'cmake / ninja / command line VC' for build. If you are not familiar with ninja, a short description here.

The nice thing about ninja is that build dependencies are pre-calculated, this makes build time of big projects like llvm/clang a lot faster. specially at incremental build, with changes in one single cpp, you may easily be an order of magnitude faster.

So, cmake runs the first time from build files in the source code, it generates build files for ninja. Then you always build using 'ninja'.
If there is any change to build files in the project, ninja detects it and automatically re runs cmake to update the build files.


Requirements
------------

0. some git client
1. MSVC 2017
2. Cmake (I have 3.13.2, but any recent version should work) https://cmake.org/
3. Ninja (I have 1.8.2, but any recent should work) https://ninja-build.org/
4. Python 2.7 ( Version 3.x will not work, must be 2.7.x) https://www.python.org/downloads/release/python-2715/

I have git, cmake and ninja in my system PATH. For MSCV and Python I use a bat file to set up environment variables. This is better because it allows to have more than one version intalled and have scripts to set environment for each version.


I put all bat files at my project root that is `C:\projects\node-dot-cpp`

I use `build-vs2017x64-env.bat` with the following:

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%

	title build

	%ComSpec% /k ""D:\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


You will need to change paths to match your system.


Build
-----

When you double click on `build-vs2017x64-env.bat` you will get a console, with python and VC command line environmnet.

First clone and checkout everything:

	(proj)> git clone -b release_70 https://github.com/llvm-mirror/llvm.git
	(proj)> cd llvm\tools
	(proj)\llvm\tools> git clone -b node-dot-cpp_70 https://github.com/node-dot-cpp/clang.git
	(proj)\llvm\tools> cd clang\tools
	(proj)\llvm\tools\clang\tools> git clone -b release_70 https://github.com/llvm-mirror/clang-tools-extra.git extra
	(proj)\llvm\tools\clang\tools> git clone https://github.com/node-dot-cpp/nodecpp-checker.git
	(proj)\llvm\tools\clang\tools> cd ..\..\..\..


Now build (release):

	(proj)> md build-llvm\release
	(proj)> cd build-llvm\release

	(proj)\build-llvm\release> cmake -DCMAKE_BUILD_TYPE=Release -G Ninja ..\..\llvm

	(proj)\build-llvm\release> ninja nodecpp-checker

	(proj)\build-llvm\release> ninja nodecpp-safe-library

	(proj)\build-llvm\release> ninja FileCheck

Both tools should end at `build-llvm\release\bin` folder, `FileCheck` is a llvm/clang tool used in automated testing.


Test
----

Now I use a second bat file (`test-env.bat`) to add tools to PATH,


	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%
	set PATH=D:\projects\node-dot-cpp\llvm\utils\lit;%PATH%
	set PATH=D:\projects\node-dot-cpp\build-llvm\release\bin;%PATH%

	cd D:\projects\node-dot-cpp\llvm\tools\clang\tools\nodecpp-checker\test\nodecpp-checker

	title test

	call %ComSpec%

Again, please change paths to match your system.
Double click on `test-env.bat` and you will get a console, with Python and our tools in the PATH.

Then, to run automated tests,

	(proj)\llvm\tools\clang\tools\nodecpp-checker\test\nodecpp-checker> llvm-lit.py .

If you are really lucky, will see a bunch of test files, with the 'PASS' legend on the left :)

To run our tool on a single file and see the output,

	(proj)\llvm\tools\clang\tools\nodecpp-checker\test\nodecpp-checker> nodecpp-checker s1.cpp --
	
Don't forget the double hypen (`--`) at the end.


Please see file `CHECHER-RUN.md` to set up the environment to run the tool over your own files or projects.

