
set PATH=%cd%\3rdparty\llvm\utils\lit;%PATH%
set PATH=%cd%\build\vs2017\Release\bin;%PATH%

llvm-lit.py test\nodecpp-checker
