@echo off
setlocal
set "ROOT=%~dp0.."
set "INCLUDE_ROOT=%ROOT%\src"
set "MODE=current"
set "REFERENCE="
if "%~1"=="regression" (
    set "MODE=regression"
    set "REFERENCE=/DSHORTCUT_PACKETS_REFERENCE"
)
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\shortcut_packets_%MODE%" mkdir "%ROOT%\build\tests\shortcut_packets_%MODE%"
pushd "%ROOT%\build\tests\shortcut_packets_%MODE%" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%INCLUDE_ROOT%" /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_shortcut_packets.c" /Fe:test_shortcut_packets.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_shortcut_packets.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
