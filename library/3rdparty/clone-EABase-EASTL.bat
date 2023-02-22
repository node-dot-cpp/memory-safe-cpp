
rmdir /S /Q EABase
rmdir /S /Q EASTL

git clone --depth 1 -b 2.09.06 https://github.com/electronicarts/EABase.git
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
cd EABase
git apply ..\EABase-2.09.06.diff
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
rmdir /S /Q .git
cd ..

git clone --depth 1 -b eastl-3.19.05 https://github.com/electronicarts/EASTL.git
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
cd EASTL
git apply ..\EASTL-3.19.05.diff
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%
rmdir /S /Q .git
cd ..
