

Quick try
=========

To run a very simple first sample, you only need to have `nodecpp-checker.exe` on your `PATH`.
If you downloaded a binary file, you can just copy it into the folder `checker/test/nodecpp-checker`. If you built it from source code, or prefer to put your binaries somewhere else, just open a console and set your env:

	set PATH=path\to\tool\exe;%PATH%

Where `path/to/tool/exe` may be full path of `checker/build/vs2017/Release/bin` if you compiled the tool with VS2017 command line tools, similar thing under Linux but for the path  `checker/build/release/bin`.

Then go to folder with test cases `checker/test/nodecpp-checker` and run:

	> cd checker\test\nodecpp-checker
	> nodecpp-checker s1-1.cpp
	> nodecpp-checker s1-4.cpp
	> nodecpp-checker s5-3.cpp
	> nodecpp-checker s8.cpp

You can run the tool over any of the `.cpp` files on that folder. However there is one important limitation in this _simple_ mode, that tests can't access the __std__ library on your system, or the real `safe_ptr.h` library, they use a small _mock_ of them found in `Inputs` folders. This is done such way to improve test stability.

Safe library and compilation database
-------------------------------------
Also important to notice is that `nodecpp-checker` will automatically pick `safe_library.json` and `compile_flags.txt` from the source folder.
Please see [CHECHER-RUN.md](CHECHER-RUN.md) to better understand how those files fit inside the process and when you may need to modify them for your specific needs.

Next
----
See [CHECHER-RUN.md](CHECHER-RUN.md) to set up the environment to run the tool over your own files or projects.

Or take a look at [CHECKER-TEST.md](CHECKER-TEST.md) to run or add automated test cases.



