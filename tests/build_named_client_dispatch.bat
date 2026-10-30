@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\named_client_dispatch" mkdir "%ROOT%\build\tests\named_client_dispatch"
pushd "%ROOT%\build\tests\named_client_dispatch" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_named_client_dispatch.c" "%ROOT%\src\std\list.c" /Fe:test_client_operations.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_client_operations.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
