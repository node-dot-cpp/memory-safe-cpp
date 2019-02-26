md 3rdparty
cd 3rdparty

git clone --depth 10 -b release_70 https://github.com/llvm-mirror/llvm.git
git clone --depth 100 -b node-dot-cpp_70 https://github.com/node-dot-cpp/clang.git
git clone --depth 10 -b release_70 https://github.com/llvm-mirror/clang-tools-extra.git

cd ..

