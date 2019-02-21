
setlocal
call test-env.bat

llvm-lit.py test\nodecpp-checker

endlocal
