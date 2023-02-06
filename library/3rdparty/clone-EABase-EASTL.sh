#!/bin/sh
set -ev

rm -Rf EABase
rm -Rf EASTL

git clone --depth 1 -b 2.09.06 https://github.com/electronicarts/EABase.git
cd EABase
git apply ../EABase-2.09.06.diff
rm -Rf .git
cd ..

git clone --depth 1 -b 3.18.00 https://github.com/electronicarts/EASTL.git
cd EASTL
git apply ../EASTL-3.18.00.diff
rm -Rf .git
cd ..
