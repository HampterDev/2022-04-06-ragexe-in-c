@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\body" mkdir "%ROOT%\build\tests\body"
pushd "%ROOT%\build\tests\body" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /Gy /Gw /MD /TC /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_body.c" "%ROOT%\tests\body_scenario.c" "%ROOT%\src\body\body.c" "%ROOT%\src\body\grid_path.c" /Fe:test_body.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_body.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
