EABase and EASTL
================

Both libraries, `3rdparty/EABase` and `3rdparty/EASTL` have been cloned _by-value_.

**EABase** folder is by-value clone of [https://github.com/electronicarts/EABase.git] initially done at tag `2.09.06`.

**EASTL** folder is by-value clone of [https://github.com/electronicarts/EASTL] initially done at tag `3.17.03` and patched with `EASTL-3.17.03.diff`.


It is done with `clone-EABase-EASLT.sh` or `clone-EABase-EASTL.bat` scripts.

Any future changes to files inside `EASTL` should preserve this procedure.
Recomemded procedure is:
* clone the initial revision in a separate folder
* apply patch and do changes there
* when work is done, make diff `git diff > ../EASTL.diff`
* re-run clone script to overwrite the _by-value_ copy

