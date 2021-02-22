

Linux Quick Start
=================


Build requirements
------------------

1. git client
2. GCC (my default is `gcc-7`, but 6 or 8 _should_ work too, for `clang` see below)
3. Make
4. Cmake (I have 3.10.2, but any recent version should work) https://cmake.org/
5. Python 2.7 ( Version 3.x will not work, must be 2.7.x) 

On Ubuntu 18.04, the following lines will make the trick:

	sudo apt update
	sudo apt install git make cmake g++ python

For Fedora 29:

	sudo yum install git-core make cmake gcc-c++ python

And for SUSE Server 15:

	sudo zypper install git-core make cmake gcc-c++ python


Simple build (Release build using Make)
---------------------------------------

You most likely already cloned this repository, but just in case you didn't, checkout `node-dot-cpp/memory-safe-cpp` with recursive submodules:

	git clone --recurse-submodules https://github.com/node-dot-cpp/memory-safe-cpp.git

Then go to the folder `memory-safe-cpp/checker`:

	cd memory-safe-cpp/checker

Then run 

	./checkout.sh

script, it will clone all llvm/clang dependencies in their required locations.
Then run

	./build.sh

script, it will configure the build using `cmake` and will build the tools. A short automated test suite should run after the build is complete. If there are errors on tests please see __Troubleshoting__

Last you can run

	sudo ./install.sh

to copy binaries to folder `/usr/local/bin`. Alternatively, if you don't have root or your system don't have a `/usr/local/bin` you can simple add the path to your env. Built binaries can be found at `checker/build/release/bin`:

	# Alternative to install.sh
	# export PATH=full/path/to/checker/build/release/bin:$PATH

Running samples
---------------

Go to folder with test cases `test/library` and run:

	cd test/library
	safememory-checker rules.cpp

You can run the tool over any of the `.cpp` files. You can add files with your own sample code on the same folder and run them.

However there are two _special_ files (`safe_library.json` and `compile_flags.txt`) on that folder that are automatically detected by the tool, and make things work easily.

Please see [CHECKER-RUN.md](CHECKER-RUN.md) to better understand how those files fit inside the process, and how set up the environment to run the tool over your own projects.

Or take a look at [CHECKER-TESTS.md](CHECKER-TESTS.md) to run or add automated test cases.



Build with clang and other build options
----------------------------------------

While `gcc` is the default option, tools can be built with other gcc versions or with `clang`.

To do that, `checker/build.sh` needs to be modified:

	# Uncomment lines below to use clang or change to other compiler
	export CC=clang-7
	export CXX=clang++-7

Is self explanatory, you can use just `clang` and `clang++` or be version specific, or `gcc-8` and `g++-8`, etc.

As  `cmake` has cached options inside the `build/release` folder, the script will delete it completelly on every new run.

I normally use `ninja` generator instead of `Makefiles`.
Other common `cmake` options are also available.
For `llvm/clang` specific options to cmake, see (https://llvm.org/docs/CMake.html)



MacOSX Quick Start
==================

MacOS has preinstalled most of the required dependencies, only missing is Cmake.

Download cmake 'dmg' from https://cmake.org/download/
Open downloaded file and drag into 'Applications', then open a command line console and run:

	sudo /Applications/CMake.app/Contents/bin/cmake-gui --install


After that you can follow generic __Simple build__ instrucctions for Linux.
But keep in mind that on MacOS C++ standard library is not always correctly detected and besides things mentioned in __Troubleshooting__ you may need to edit file `checker/test/library/compile_flags.txt` and add 4 lines similar to the following at the end:

	-isystem
	/Library/Developer/CommandLineTools/usr/include/c++/v1
	-isysroot
	/Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk

Please verify those are the correct paths for the standard library includes on your system, or change them accordingly.

Gcc is not needed on mac, and cmake will default to `clang` automatically.


CentOS Quick Start
==================

CentOS 7 needs some special steps to update cmake and GCC:

	sudo yum install centos-release-scl
	sudo yum install devtoolset-7-gcc-c++
	sudo yum install epel-release
	sudo yum install cmake3
	sudo yum install git-core make python wget nano

	scl enable devtoolset-7 bash

Then you need to edit file `checker/build.sh` and update `cmake` for `cmake3`:

	cmake3 -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release .......


After that you can follow generic Linux __Simple build__.


Keep in mind that every time you open a new console to use the tool, you will need to run:

	scl enable devtoolset-7 bash

To bring into the environment the updated gcc.



Windows Quick Start
===================

Requirements
------------

1. git command line client 
2. Visual Studio 2017 (C++ command line tools)
3. Cmake (I have 3.13.2, but any recent version should work) https://cmake.org/
4. Python 2.7 ( Version 3.x will not work, must be 2.7.x) https://www.python.org/downloads/release/python-2715/

All git, Cmake and Python 2.7 must be in your `PATH` environment so scripts can call them.

Simple build (Release build with VS2017 command line tools, using msbuild)
--------------------------------------------------------------------------

You most likely already cloned this repository, but just in case you didn't, checkout `node-dot-cpp/memory-safe-cpp` with recursive submodules:

	git clone --recurse-submodules https://github.com/node-dot-cpp/memory-safe-cpp.git

Then go to the folder `memory-safe-cpp/checker`:

	cd memory-safe-cpp\checker

Then run 

	checkout.bat

script, it will clone all llvm/clang dependencies in their required locations.
Then run

	build.bat

script, it will configure the build using `cmake` and will build the tools. A short automated test suite should run after the build is complete. If there are errors on tests please see __Troubleshoting__

To add the binaries to your path (only on your current console), run `set-path.bat` script. Keep in mind you will need to run this script on every new console.

Now you can follow __Running samples__ from generic Linux section.


Troubleshooting
===============

There are some common causes to errors on automated test cases.

Tests under the `check` folder _should_ be self contained and not to fail. If you have errors there, then please fill a bug report.
Tests under `library` tend to fail more easily, usually because of environment setup. Most of the time its easy to work around with some basic guidelines.

Go to the library folder and run the tool

	cd test/library
	safememory-checker all-good.cpp

Then you may get:

1. Errors about not finding C++ standard include headers (like `<vector>`). To fix this, you will need to edit `compile_flags.txt` in the same `test/library` foler, and manually add the path to your system C++ library include.

2. Errors about not finding `foundation.h` or similar. This is most likely because there are missing submodules of the `memory-safe-cpp` repo. Please verify folders `library/src/iibmalloc` and `library/src/iibmalloc/src/foundation` are not empty. If they are please checkout repository again with `--recurse-submodules` flag set.

3. Other missing includes. You may try to find the files, and add their paths to `test/library/compile_flags.txt`



