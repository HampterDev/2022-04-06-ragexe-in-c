@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\swing_pace" mkdir "%ROOT%\build\tests\swing_pace"
pushd "%ROOT%\build\tests\swing_pace" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /Gy /Gw /MD /TC /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_swing_pace.c" "%ROOT%\src\std\swing_pace.c" /Fe:test_swing_pace.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_swing_pace.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
