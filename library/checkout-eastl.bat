
cd 3rdparty

rmdir /S /Q EABase
rmdir /S /Q EASTL

git clone --depth 1 -b 2.09.12 https://github.com/electronicarts/EABase.git
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

git clone --depth 1 -b 3.17.03 https://github.com/electronicarts/EASTL.git
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd EASTL
git apply ..\EASTL-3.17.03.diff
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd ..\..

