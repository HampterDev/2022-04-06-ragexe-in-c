@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\predicted" mkdir "%ROOT%\build\tests\predicted"
pushd "%ROOT%\build\tests\predicted" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /Gy /Gw /MD /TC /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_predicted.c" "%ROOT%\src\world\predicted.c" "%ROOT%\src\body\body.c" "%ROOT%\src\body\grid_path.c" /Fe:test_predicted.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_predicted.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
