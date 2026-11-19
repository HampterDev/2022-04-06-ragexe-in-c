@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\delivery" mkdir "%ROOT%\build\tests\delivery"
pushd "%ROOT%\build\tests\delivery" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_delivery.c" "%ROOT%\src\delivery\delivery.c" /Fe:test_delivery.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_delivery.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
