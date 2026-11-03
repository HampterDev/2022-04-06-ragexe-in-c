@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\text_runs" mkdir "%ROOT%\build\tests\text_runs"
pushd "%ROOT%\build\tests\text_runs" || exit /b 1
cl /nologo /W4 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_text_runs.c" "%ROOT%\src\render\text_runs.c" "%ROOT%\src\core\utf8.c" /Fe:test_text_runs.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_text_runs.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
