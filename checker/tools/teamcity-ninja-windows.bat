rem Build script used by teamcity CI
rem This script requires a properly configured environment, in particular
rem vcvars64.bat from Visual Studio and sccache.exe and ninja.exe in the path


cd 3rdparty
if not exist "llvm-project\" (
    git clone --depth 1 --branch llvmorg-11.1.0 https://github.com/llvm/llvm-project.git
    @if ERRORLEVEL 1 exit /b %ERRORLEVEL%

    cd llvm-project
    git apply ..\llvm-project_llvmorg-11.1.0.diff
    @if ERRORLEVEL 1 exit /b %ERRORLEVEL%

    cd ..
)

cd ..

if not exist "build\" (
    mkdir build
)

cd build

if not exist "teamcity\" (
    mkdir teamcity
)

cd teamcity

cmake -DCMAKE_CXX_COMPILER_LAUNCHER="sccache" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_SAFEMEMORY_SOURCE_DIR=%cd%\..\.. -G Ninja ..\..\3rdparty\llvm-project\llvm
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target check-safememory-tools
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd ..\..
