@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\max_job_level" mkdir "%ROOT%\build\tests\max_job_level"
pushd "%ROOT%\build\tests\max_job_level" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /TC /std:c11 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" "%ROOT%\tests\test_max_job_level.c" /Fe:test_max_job_level.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_max_job_level.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
