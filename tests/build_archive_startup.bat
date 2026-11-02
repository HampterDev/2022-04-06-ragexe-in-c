@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\archive_startup" mkdir "%ROOT%\build\tests\archive_startup"
pushd "%ROOT%\build\tests\archive_startup" || exit /b 1
cl /nologo /W1 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_archive_startup.c" "%ROOT%\src\core\archive_startup.c" /Fe:test_archive_startup.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_archive_startup.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
