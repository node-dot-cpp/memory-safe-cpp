
To compile and run an application using memory-safe-cpp:

0. Check out memory-safe-cpp project recursively (yes, it refers to a few other modules)

1. Compiler/platform/language:

  - Linux: GCC and CLANG (latest versions)
  - Windows: MSVC (latest version)

Currently only 64-bit platforms are supported.
C++17 is required.

1. Add paths to the following libraries:

  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/foundation/include;
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/foundation/3rdparty/fmt/include;
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src;
  - [MEMORY-SAFE-CPP]/library/src;
  (for GCC and CLANG add respective -I compiler option; for MSVC: update VC++ directories -> include directories)
  
2. Add the following source files:
  
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/foundation/3rdparty/fmt/src/format.cc 
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/foundation/src/std_error.cpp 
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/foundation/src/safe_memory_error.cpp 
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/foundation/src/log.cpp 
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/foundation/src/tagged_ptr_impl.cpp 
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/foundation/src/cpu_exceptions_translator.cpp
  
  Also, on Linux:
  
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/page_allocator_linux.cpp 
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/iibmalloc_linux.cpp
  
  Respectively, on Windows,
  
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/page_allocator_windows.cpp 
  - [MEMORY-SAFE-CPP]/library/src/iibmalloc/src/iibmalloc_windows.cpp
  
3. Add the following #include directives to code:
  - #include <fmt/format.h>
  - #include <foundation.h>
  - #include <nodecpp_assert.h>
  - #include <iibmalloc.h>
  - #include <safe_ptr.h>
  
4. Build with a compiler (in case of compiler errors like missing include files or unresolved calls, etc check above steps first, especially step 0) 

5. Proceed to safety checks with Code Checker (see [MEMORY-SAFE-CPP]/checker/doc/CHECKER-QUICK-START.md for details
