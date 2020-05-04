
rmdir /S /Q build\vs2017
mkdir build\vs2017
cd build\vs2017

cmake -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_CHECKER_SOURCE_DIR=%cd%\..\.. -Thost=x64 -G "Visual Studio 15 2017 Win64" ..\..\3rdparty\llvm
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target nodecpp-checker --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target nodecpp-instrument --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target nodecpp-safe-library --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target check-nodecpp-tools --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd ..\..

