
setlocal
set PATH=%cd%\3rdparty\llvm-project\llvm\utils\lit;%PATH%
set PATH=%cd%\build\vs\Release\bin;%PATH%

llvm-lit.py test

endlocal
