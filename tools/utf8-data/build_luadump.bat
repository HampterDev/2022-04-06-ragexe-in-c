@echo off
REM build_luadump.bat - build build\utf8-data\luadump.exe from vendor\lua-5.1.4 (32-bit,
REM like the client, so it loads the 32-bit Lua 5.1 bytecode in the .lub files).
setlocal
set "ROOT=%~dp0..\.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OUT=%ROOT%\build\utf8-data"
if not exist "%OUT%\obj" mkdir "%OUT%\obj"
cl /nologo /O2 /MD /W3 /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\vendor\lua-5.1.4\src" /Fo"%OUT%\obj\\" ^
   "%ROOT%\tools\utf8-data\luadump.c" "%ROOT%\vendor\lua-5.1.4\src\*.c" /Fe"%OUT%\luadump.exe"
exit /b %errorlevel%
