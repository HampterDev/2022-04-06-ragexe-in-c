@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%\src"
set "MODE=current"
set "EXTRA=/WX"
set "TRACE="
if "%~2"=="trace" set "TRACE=trace.bin"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
for %%C in (1) do (
    call :variant %%C
    if errorlevel 1 (exit /b 1) else if not errorlevel 0 exit /b 1
)
exit /b 0

:variant
if not exist "%ROOT%\build\tests\packet_dispatch_%MODE%_%1" mkdir "%ROOT%\build\tests\packet_dispatch_%MODE%_%1"
pushd "%ROOT%\build\tests\packet_dispatch_%MODE%_%1" || exit /b 1
cl /nologo /W1 %EXTRA% /we4013 /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%SOURCE%" /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_packet_dispatch.c" "%ROOT%\tests\packet_dispatch_sinks.c" "%SOURCE%\game\packet_dispatch.c" "%SOURCE%\game\packet_dispatch2.c" /Fe:test_packet_dispatch.exe /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'"
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_packet_dispatch.exe %TRACE%
set "CODE=%errorlevel%"
popd
exit /b %CODE%
