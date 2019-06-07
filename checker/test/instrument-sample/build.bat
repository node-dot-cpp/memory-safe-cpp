
del instrument_this.exe 
del instrument_this.instrumented.exe
del instrument_this.instrumented.cpp

set CLANG_OPTS=-isystem ../../../library/src/iibmalloc/src -isystem ../../../library/src -isystem ../../../library/src/iibmalloc/src/foundation/include -isystem ../../../library/src/iibmalloc/src/foundation/3rdparty/fmt/include -std=c++17 -g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-empty-body -fexceptions -fnon-call-exceptions -DNDEBUG -O2 -flto -fuse-ld=lld
set LIBRARY_FILES=../../../library/src/safe_ptr.cpp ../../../library/src/iibmalloc/src/foundation/3rdparty/fmt/src/format.cc ../../../library/src/iibmalloc/src/foundation/src/log.cpp ../../../library/src/iibmalloc/src/foundation/src/cpu_exceptions_translator.cpp ../../../library/src/iibmalloc/src/foundation/src/std_error.cpp ../../../library/src/iibmalloc/src/foundation/src/safe_memory_error.cpp ../../../library/src/iibmalloc/src/foundation/src/tagged_ptr_impl.cpp ../../../library/src/iibmalloc/src/page_allocator_windows.cpp ../../../library/src/iibmalloc/src/iibmalloc_windows.cpp

clang++ instrument_this.cpp %LIBRARY_FILES% %CLANG_OPTS% -o instrument_this.exe
nodecpp-instrument -o instrument_this.instrumented.cpp instrument_this.cpp -- %CLANG_OPTS%
clang++ instrument_this.instrumented.cpp %LIBRARY_FILES% %CLANG_OPTS% -o instrument_this.instrumented.exe


