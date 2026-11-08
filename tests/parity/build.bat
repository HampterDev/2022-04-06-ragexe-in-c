@echo off
rem Builds the parity harness into build\parity: parity.dll (loaded into a game client) and
rem parity_launch.exe (starts a client with it). Both are 32-bit, like the clients.
setlocal
set "ROOT=%~dp0..\.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OUT=%ROOT%\build\parity"
if not exist "%OUT%" mkdir "%OUT%"
pushd "%OUT%" || exit /b 1
cl /nologo /W4 /WX /O2 /MD /LD /D_CRT_SECURE_NO_WARNINGS "%~dp0parity.c" /Fe:parity.dll ^
   /link /OPT:REF ws2_32.lib user32.lib advapi32.lib gdi32.lib
if errorlevel 1 (popd & exit /b 1)
cl /nologo /W4 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS "%~dp0parity_launch.c" /Fe:parity_launch.exe ^
   /link /OPT:REF /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker'"
if errorlevel 1 (popd & exit /b 1)
popd
echo parity harness built in %OUT%
