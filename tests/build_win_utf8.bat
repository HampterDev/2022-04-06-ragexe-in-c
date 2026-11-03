@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\win_utf8" mkdir "%ROOT%\build\tests\win_utf8"
pushd "%ROOT%\build\tests\win_utf8" || exit /b 1
cl /nologo /c /W4 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /FI"%ROOT%\tests\win_utf8_fixture.h" "%ROOT%\src\platform\win_utf8.c" /Fo:win_utf8_test.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /O2 /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_win_utf8.c" win_utf8_test.obj /Fe:test_win_utf8.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_win_utf8.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
