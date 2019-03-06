

Build with DEBUG on Windows
---------------------------

Building on Windows can get tricky sometimes.
I normally build under Windows using 'cmake / ninja / command line VC', ninja (https://ninja-build.org/) is a lot faster on incremental builds than `msbuild`.

On Windows I use `build-env.bat` with the following:

	title build

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%

	%ComSpec% /k ""C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


You will need to change paths to match your system, but using environment scripts helps a lot with having a consistent environment on Windows.
Then you can use `cmake-ninja-debug.bat` to run cmake, and then build the tools:

	ninja nodecpp-checker
	ninja nodecpp-safe-library
	ninja check-nodecpp-checker


For testing I use a separate `test-debug-env.bat` file to set up environmnet to run the tool and tests.

	title test

	set PATH=C:\Python27;C:\Python27\Scripts;%PATH%
	set PATH=%cd%\3rdparty\llvm\utils\lit;%PATH%
	set PATH=%cd%\build\debug\bin;%PATH%

	%ComSpec% /k ""C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat""


Then, to run manually run automated tests:

	llvm-lit.py test

To run our tool on a single test file and see the output,

	cd test\sample
	nodecpp-checker rules.cpp 
	

Next
====

Or you can see [CHECKER-RUN.md](CHECKER-RUN.md) for details on set up the environment to run the tool over your own files or projects.

Or take a look at [CHECKER-TESTS.md](CHECKER-TESTS.md) for details on run or add automated test cases.


 
