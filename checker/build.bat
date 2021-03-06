
rmdir /S /Q build\vs
mkdir build\vs
cd build\vs

cmake -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_SAFEMEMORY_SOURCE_DIR=%cd%\..\.. -Thost=x64 -G "Visual Studio 16 2019" ..\..\3rdparty\llvm-project\llvm
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target safememory-checker --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target safememory-instrument --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target safememory-library-db --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target safememory-odr --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --target check-safememory-tools --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd ..\..

