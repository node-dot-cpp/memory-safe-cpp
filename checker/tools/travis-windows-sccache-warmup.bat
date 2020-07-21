rem travis build tend to time out when building tools under windows.
rem we are using sccache to try speed up build, but when build times out
rem travis won't upload the cache, making a chicken-egg situation.
rem Cache is not uploaded because build did timeout; build timeout because there is no cache to download.

rem So we use this build, that is smaller and is very likely to finish,
rem to populate part of the cache. Hopefully this build will complete, cache will be uploaded,
rem and next build of the tools will have at least a partial cache so there will be some speed up,
rem and its build will complete and upload the full cache.


rem mb: Travis-ci uses git-bash to execute scripts under Windows.
rem But vcvars needs to be called under cmd.exe, so this .bat file forces
rem git-bash to launch a cmd.exe to execute this.


cd checker
call checkout.bat
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
cd ..

mkdir build
cd build

set PATH=C:\ProgramData\chocolatey\lib\sccache\tools\sccache-0.2.12-x86_64-pc-windows-msvc;%PATH%

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake -DCMAKE_CXX_COMPILER_LAUNCHER="sccache.exe" -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_CHECKER_SOURCE_DIR=%cd%\..\checker -G Ninja ..\checker\3rdparty\llvm
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja clangAST
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja clangBasic
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja clangDriver
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja clangFrontend
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja clangLex
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja clangParse
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja clangSema
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja clangTooling
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
