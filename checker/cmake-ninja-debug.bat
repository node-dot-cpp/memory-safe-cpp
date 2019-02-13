md build\debug
cd build\debug

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja ..\..\llvm
