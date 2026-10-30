@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\thread_text" mkdir "%ROOT%\build\tests\thread_text"
pushd "%ROOT%\build\tests\thread_text" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /MD /TC /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_thread_text.c" "%ROOT%\src\core\thread_text.c" /Fe:test_thread_text.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_thread_text.exe %1
set "CODE=%errorlevel%"
popd
exit /b %CODE%
