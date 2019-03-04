

Linux Quick Start
=================


Build requirements
------------------

0. git client
1. GCC 7
2. Make
3. Cmake (I have 3.10.2, but any recent version should work) https://cmake.org/
4. Python 2.7 ( Version 3.x will not work, must be 2.7.x) 

On Ubuntu, the following lines will make the trick:

	sudo apt update
	sudo apt install git make cmake g++-7 python


Simple build (Release build using Make)
---------------------------------------

You most likely already cloned this repository, but just in case you didn't, checkout `node-dot-cpp/memory-safe-cpp` with recursive submodules:

	git clone --recurse-submodules https://github.com/node-dot-cpp/memory-safe-cpp.git

Then go to the folder `memory-safe-cpp/checker`:

	cd memory-safe-cpp/checker

First run `./checkout.sh` script, it will clone all llvm/clang dependencies in their required locations.
Then run `./build.sh` script, it will configure the build using `cmake` and will build the tools. A short automated test suite should run after the build is complete. If there are errors on tests on folder `samples` but not on `regression` there is most likely an environment issue, most common problem is a missing submodule of `memory-safe-cpp` repository.

Last you can run `sudo ./install.sh` to copy binaries to folder `/usr/local/bin`. If you don't have root or your system don't have a `/usr/local/bin` you can simple add the path to your env:

	export PATH=full/path/to/checker/build/release/bin:$PATH



Running samples
---------------

Go to folder with test cases `test/samples` and run:

	cd test/samples
	nodecpp-checker rules.cpp

You can run the tool over any of the `.cpp` files. You can add files with your own sample code on the same folder and run them.

However there are two _special_ files (`safe_library.json` and `compile_flags.txt`) on that folder that are automatically detected by the tool, and make things work easily.

Please see [CHECHER-RUN.md](CHECHER-RUN.md) to better understand how those files fit inside the process, and how set up the environment to run the tool over your own projects.

Or take a look at [CHECKER-TEST.md](CHECKER-TEST.md) to run or add automated test cases.

