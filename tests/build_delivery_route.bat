@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\delivery_route" mkdir "%ROOT%\build\tests\delivery_route"
pushd "%ROOT%\build\tests\delivery_route" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_delivery_route.c" "%ROOT%\src\delivery\delivery_route.c" /Fe:test_delivery_route.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_delivery_route.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
