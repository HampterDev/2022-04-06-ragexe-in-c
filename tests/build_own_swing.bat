@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\own_swing" mkdir "%ROOT%\build\tests\own_swing"
pushd "%ROOT%\build\tests\own_swing" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /Gy /Gw /MD /TC /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_own_swing.c" "%ROOT%\src\world\own_swing.c" "%ROOT%\src\world\act_track.c" /Fe:test_own_swing.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_own_swing.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
