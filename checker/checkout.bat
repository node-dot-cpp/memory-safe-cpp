
cd 3rdparty

rmdir /S /Q llvm
rmdir /S /Q clang
rmdir /S /Q clang-tools-extra

git clone --depth 10 -b release_70 https://github.com/llvm-mirror/llvm.git
git clone --depth 10 -b release_70 https://github.com/llvm-mirror/clang.git

cd clang
git apply ..\clang_release_70.diff

cd ..\..

