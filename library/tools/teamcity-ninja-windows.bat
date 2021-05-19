rem Build script used by teamcity CI
rem This script requires a properly configured environment, in particular
rem vcvars64.bat from Visual Studio and sccache.exe and ninja.exe in the path

if not exist "build\" (
    mkdir build
)

cd build

if not exist "teamcity\" (
    mkdir teamcity
)

cd teamcity

cmake -DCMAKE_CXX_COMPILER_LAUNCHER="sccache" -DCMAKE_BUILD_TYPE=Release -G Ninja ..\..
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build .
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ctest --output-on-failure
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd ..\..
