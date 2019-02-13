
git clone -b release_70 https://github.com/llvm-mirror/llvm.git
cd llvm\tools
git clone -b node-dot-cpp_70 https://github.com/node-dot-cpp/clang.git
cd clang\tools
git clone -b release_70 https://github.com/llvm-mirror/clang-tools-extra.git extra
git clone https://github.com/node-dot-cpp/clang-tools-nodecpp.git nodecpp
cd ..\..\..\..

