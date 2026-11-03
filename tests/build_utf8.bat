@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\utf8" mkdir "%ROOT%\build\tests\utf8"
pushd "%ROOT%\build\tests\utf8" || exit /b 1
cl /nologo /W4 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_utf8.c" "%ROOT%\src\core\utf8.c" /Fe:test_utf8.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_utf8.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
