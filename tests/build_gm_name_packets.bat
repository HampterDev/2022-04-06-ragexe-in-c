@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\gm_name_packets" mkdir "%ROOT%\build\tests\gm_name_packets"
pushd "%ROOT%\build\tests\gm_name_packets" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_gm_name_packets.c" "%ROOT%\src\platform\clock_win32.c" "%ROOT%\src\core\cstr.c" /Fe:test_gm_name_packets.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_gm_name_packets.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
