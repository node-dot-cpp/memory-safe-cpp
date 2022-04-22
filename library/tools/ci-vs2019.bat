rem script to build with Visual Studio
rem run from root as 'tools\ci-msvc2019'

rmdir /S /Q library\build\vs2019
mkdir library\build\vs2019
cd library\build\vs2019

cmake -DSAFEMEMORY_TEST=ON -G "Visual Studio 16 2019" ..\..
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build . --config Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ctest --output-on-failure -C Release
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
