@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\agency_packets" mkdir "%ROOT%\build\tests\agency_packets"
pushd "%ROOT%\build\tests\agency_packets"
cl /nologo /W4 /WX /std:c11 /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" "%ROOT%\tests\test_agency_packets.c" /Fe:test_agency_packets.exe
if errorlevel 1 (popd & exit /b 1)
test_agency_packets.exe %1
if errorlevel 1 (popd & exit /b 1)
cl /nologo /W1 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_agency_results.c" /Fe:test_agency_results.exe /link /OPT:REF user32.lib
if errorlevel 1 (popd & exit /b 1)
test_agency_results.exe
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%
