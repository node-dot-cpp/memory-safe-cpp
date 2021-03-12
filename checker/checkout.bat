
cd 3rdparty

rmdir /S /Q llvm
rmdir /S /Q clang
rmdir /S /Q clang-tools-extra
rmdir /S /Q llvm-project

git clone --depth 1 --branch llvmorg-11.1.0 https://github.com/llvm/llvm-project.git
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd llvm-project
git apply ..\llvm-project_llvmorg-11.1.0.diff
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd ..\..

