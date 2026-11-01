@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\actor_actions" mkdir "%ROOT%\build\tests\actor_actions"
pushd "%ROOT%\build\tests\actor_actions" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_actor_actions.c" "%ROOT%\src\actor\action.c" "%ROOT%\src\actor\head_extent.c" /Fe:test_actor_actions.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_actor_actions.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
