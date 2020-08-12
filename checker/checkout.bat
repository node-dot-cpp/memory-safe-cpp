
cd 3rdparty

rmdir /S /Q llvm
rmdir /S /Q clang
rmdir /S /Q clang-tools-extra

git clone --depth 1 -b release_90 https://github.com/llvm-mirror/llvm.git
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

git clone --depth 1 -b release_90 https://github.com/llvm-mirror/clang.git
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd clang
git apply ..\clang_release_90.diff
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd ..\..

