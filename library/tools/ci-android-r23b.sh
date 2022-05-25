#!/bin/sh
set -ev

rm -Rf library/build/android-r23b
mkdir -p library/build/android-r23b
cd library/build/android-r23b

# env variable ANDROID_NDK should point to the root folder where NDKs are installed

cmake -DCMAKE_BUILD_TYPE=Release -DSAFEMEMORY_TEST=ON -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-23 -DANDROID_NDK=${ANDROID_NDK}/android-ndk-r23b -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/android-ndk-r23b/build/cmake/android.toolchain.cmake -G Ninja ../..

cmake --build .

# is cross compile, don't run tests here
# ctest --output-on-failure

