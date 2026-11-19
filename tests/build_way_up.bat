@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\way_up" mkdir "%ROOT%\build\tests\way_up"
pushd "%ROOT%\build\tests\way_up" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_way_up.c" "%ROOT%\src\std\way_up.c" /Fe:test_way_up.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_way_up.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
