@echo off
REM tools\granny_export\build.bat -> build\granny_export.exe (32-bit, links the game's Granny 2.1).
REM Run build.bat first: it builds zlib into build\vendor\imglibs.lib.
setlocal
set "ROOT=%~dp0..\.."
set "BUILD=%ROOT%\build"
if not exist "%BUILD%\vendor\imglibs.lib" ( echo Run build.bat first. & exit /b 1 )
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%BUILD%\obj_tools" mkdir "%BUILD%\obj_tools"
cl /nologo /std:c11 /O2 /MD /W3 /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\stb" ^
 "%~dp0granny_export.c" /Fo:"%BUILD%\obj_tools\\" /Fe:"%BUILD%\granny_export.exe" ^
 /link "%ROOT%\lib\granny2.lib" "%BUILD%\vendor\imglibs.lib" /SUBSYSTEM:CONSOLE /MACHINE:X86
if errorlevel 1 ( echo *** granny_export build FAILED *** & exit /b 1 )
echo Done -^> "%BUILD%\granny_export.exe"
