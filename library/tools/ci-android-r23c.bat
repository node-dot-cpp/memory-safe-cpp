rem script to build with Visual Studio
rem run from root as 'tools\ci-msvc2019'

rmdir /S /Q library\build\android-r23c
mkdir library\build\android-r23c
cd library\build\android-r23c

rem env variable ANDROID_SDK_HOME should point to the root Sdk folder

cmake -DSAFEMEMORY_TEST=ON -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI=arm64-v8a -DANDROID_NDK=%ANDROID_SDK_HOME%\ndk\23.2.8568313 -DCMAKE_TOOLCHAIN_FILE=%ANDROID_SDK_HOME%\ndk\23.2.8568313\build\cmake\android.toolchain.cmake -G Ninja ..\..
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake --build .
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

rem we don't test here, since we must deploy on (virtual) device
rem ctest --output-on-failure -C Release
rem @if ERRORLEVEL 1 exit /b %ERRORLEVEL%
