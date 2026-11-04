@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\startup_settings" mkdir "%ROOT%\build\tests\startup_settings"
pushd "%ROOT%\build\tests\startup_settings" || exit /b 1
cl /nologo /std:c11 /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_startup_settings.c" "%ROOT%\src\platform\win_utf8.c" /Fe:test_startup_settings.exe /link /OPT:REF advapi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_startup_settings.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
