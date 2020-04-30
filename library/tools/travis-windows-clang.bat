rem mb: Travis-ci uses git-bash to execute scripts under Windows.
rem But vcvars needs to be called under cmd.exe, so this .bat file forces
rem git-bash to launch a cmd.exe to execute this.

mkdir build
cd build

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

set CC=clang-cl.exe
set CXX=clang-cl.exe

cmake -DCMAKE_BUILD_TYPE=Release -G Ninja ..
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build .
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ctest --output-on-failure --exclude-regex foundation
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

