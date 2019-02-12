
set PATH=%cd%\llvm\utils\lit;%PATH%
set PATH=%cd%\build\vs2017\Release\bin;%PATH%

llvm-lit.py llvm\tools\clang\tools\nodecpp\test\nodecpp-checker
