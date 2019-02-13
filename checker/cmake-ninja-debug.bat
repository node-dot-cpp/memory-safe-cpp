md build\debug
cd build\debug

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_NODECPP_SOURCE_DIR=%cd%\..\..\nodecpp -G Ninja ..\..\3rdparty\llvm
