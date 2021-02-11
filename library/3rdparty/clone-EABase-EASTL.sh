#!/bin/sh
set -ev

rm -Rf EABase
rm -Rf EASTL

git clone --depth 1 -b 2.09.06 https://github.com/electronicarts/EABase.git
cd EABase
rm -Rf .git
cd ..

git clone --depth 1 -b 3.17.03 https://github.com/electronicarts/EASTL.git
cd EASTL
git apply ../EASTL-3.17.03.diff
rm -Rf .git
cd ..
