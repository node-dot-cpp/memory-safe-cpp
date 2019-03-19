

Linux Quick Start
=================


Build requirements
------------------

1. git client
2. GCC (my default is `gcc-7`, but 6 or 8 _should_ work too, for `clang` see below)
3. Make
4. Cmake (I have 3.10.2, but any recent version should work) https://cmake.org/
5. Python 2.7 ( Version 3.x will not work, must be 2.7.x) 

On Ubuntu, the following lines will make the trick:

	sudo apt update
	sudo apt install git make cmake g++ python


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

script, it will configure the build using `cmake` and will build the tools. A short automated test suite should run after the build is complete. If there are errors on tests on folder `samples` but not on `regression` there is most likely an environment issue, most common problem is a missing submodule of `memory-safe-cpp` repository.

Last you can run

	sudo ./install.sh

to copy binaries to folder `/usr/local/bin`. Alternatively, if you don't have root or your system don't have a `/usr/local/bin` you can simple add the path to your env. Built binaries can be found at `checker/build/release/bin`:

	# Alternative to install.sh
	# export PATH=full/path/to/checker/build/release/bin:$PATH

Running samples
---------------

Go to folder with test cases `test/sample` and run:

	cd test/sample
	nodecpp-checker rules.cpp

You can run the tool over any of the `.cpp` files. You can add files with your own sample code on the same folder and run them.

However there are two _special_ files (`safe_library.json` and `compile_flags.txt`) on that folder that are automatically detected by the tool, and make things work easily.

Please see [CHECKER-RUN.md](CHECKER-RUN.md) to better understand how those files fit inside the process, and how set up the environment to run the tool over your own projects.

Or take a look at [CHECKER-TESTS.md](CHECKER-TESTS.md) to run or add automated test cases.


Build with clang and other build options
----------------------------------------

While `gcc` is the default option, tools can be built with other gcc versions or with `clang`.

To do that, `build.sh` needs to be modified:

	# Uncomment lines below to use clang or change to other compiler
	export CC=clang-7
	export CXX=clang++-7

Is self explanatory, you can use just `clang` and `clang++` or be version specific, or `gcc-8` and `g++-8`, etc.

When you change the value in the script (or any other `cmake` option), you need to remove the entire `build/release` folder as `cmake` has cached the options inside and will fail to run.

I normally use `ninja` generator instead of `Makefiles`.
Other common `cmake` options are also available.
For `llvm/clang` specific options to cmake, see (https://llvm.org/docs/CMake.html)


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

Checkout `node-dot-cpp/memory-safe-cpp`, if you haven't done it already.
Open a console from 'x64 Native Tools Command Prompt for VS 2017' (from Windows start menu, under Visual Studio 2017 folder) and go to the recently checked out folder `memory-safe-cpp/checker`

First run `checkout.bat` script, it will clone all llvm/clang dependencies in their required locations.
Then run `build.bat` script, it will configure the build using `cmake` and will build the tools.
A short automated test suite should run after the build is complete.
If there are errors on tests on folder `samples` but not on `regression` there is most likely an environment issue, most common problem is a missing submodule of `memory-safe-cpp` repository.

To add the binaries to your path (only on your current console), run `set-path.bat` script. Keep in mind you will need to run this script on every new console.


Running samples
---------------

See __Running samples__ from linux section.

