#!/bin/sh
set -ev

cd 3rdparty

rm -Rf EABase
rm -Rf EASTL

git clone --depth 1 -b 2.09.06 https://github.com/electronicarts/EABase.git
git clone --depth 1 -b 3.17.03 https://github.com/electronicarts/EASTL.git

cd EASTL
git apply ../EASTL-3.17.03.diff

cd ../..