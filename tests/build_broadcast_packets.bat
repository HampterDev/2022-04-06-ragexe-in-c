@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%\src"
set "MODE=current"
set "EXTRA="
set "HANDLER=%ROOT%\src\game\msg_broadcast.c"
set "STRINGS="
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\broadcast_packets_%MODE%" mkdir "%ROOT%\build\tests\broadcast_packets_%MODE%"
pushd "%ROOT%\build\tests\broadcast_packets_%MODE%" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /MD /TC /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %EXTRA% /I"%SOURCE%" "%ROOT%\tests\test_broadcast_packets.c" "%HANDLER%" "%SOURCE%\session\character_name.c" %STRINGS% /Fe:test_broadcast_packets.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_broadcast_packets.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
