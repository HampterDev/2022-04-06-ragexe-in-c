@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\record_wait" mkdir "%ROOT%\build\tests\record_wait"
pushd "%ROOT%\build\tests\record_wait" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_record_wait.c" "%ROOT%\src\world\record_wait.c" /Fe:test_record_wait.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_record_wait.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
