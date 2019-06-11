
del zombie_this.exe 
del zombie_this.instrumented.cpp
del zombie_this.instrumented.exe

del zombie_reference.exe 
del zombie_reference.instrumented.cpp
del zombie_reference.instrumented.exe

set CLANG_OPTS=-isystem ../../../library/src/iibmalloc/src -isystem ../../../library/src -isystem ../../../library/src/iibmalloc/src/foundation/include -isystem ../../../library/src/iibmalloc/src/foundation/3rdparty/fmt/include -std=c++17 -fexceptions -fnon-call-exceptions -DNDEBUG -O2 -fuse-ld=lld -flto
set LIBRARY_FILES=../../../library/src/safe_ptr.cpp ../../../library/src/iibmalloc/src/foundation/3rdparty/fmt/src/format.cc ../../../library/src/iibmalloc/src/foundation/src/log.cpp ../../../library/src/iibmalloc/src/foundation/src/cpu_exceptions_translator.cpp ../../../library/src/iibmalloc/src/foundation/src/std_error.cpp ../../../library/src/iibmalloc/src/foundation/src/safe_memory_error.cpp ../../../library/src/iibmalloc/src/foundation/src/tagged_ptr_impl.cpp ../../../library/src/iibmalloc/src/page_allocator_windows.cpp ../../../library/src/iibmalloc/src/iibmalloc_windows.cpp

clang++ zombie_this.cpp -o zombie_this.exe %LIBRARY_FILES% %CLANG_OPTS%
nodecpp-instrument -o zombie_this.instrumented.cpp zombie_this.cpp -- %CLANG_OPTS%
clang++ zombie_this.instrumented.cpp -o zombie_this.instrumented.exe %LIBRARY_FILES% %CLANG_OPTS% 

clang++ zombie_reference.cpp -o zombie_reference.exe %LIBRARY_FILES% %CLANG_OPTS%
nodecpp-instrument -o zombie_reference.instrumented.cpp zombie_reference.cpp -- %CLANG_OPTS%
clang++ zombie_reference.instrumented.cpp -o zombie_reference.instrumented.exe %LIBRARY_FILES% %CLANG_OPTS% 

