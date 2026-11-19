@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\clock_sync" mkdir "%ROOT%\build\tests\clock_sync"
pushd "%ROOT%\build\tests\clock_sync" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_clock_sync.c" "%ROOT%\src\std\clock_sync.c" /Fe:test_clock_sync.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_clock_sync.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
