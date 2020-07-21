rem travis build tend to time out on building tools under windows.
rem we are using sccache to speed up build, but when build times out
rem travis won't upload the cache, then the cache won't speed up because
rem it can't build the cache.

rem So we use this build, that is smaller and is very likely finish,
rem to populate the cache, next when the full build runs, there is some
rem cache and full build runs fast enought so it doesn't time out.


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

ninja nodecpp-safe-library
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

