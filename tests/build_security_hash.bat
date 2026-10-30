@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\security_hash" mkdir "%ROOT%\build\tests\security_hash"
pushd "%ROOT%\build\tests\security_hash" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_security_hash_owner.c" /Fe:test_security_hash_owner.exe /link /OPT:REF advapi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_security_hash_owner.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
