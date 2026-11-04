@echo off
REM Usage: symbols.bat OBJDIR OUTFILE
REM Writes the `dumpbin /symbols` listing of every object in OBJDIR (build.bat's build\obj,
REM build\obj_x64, ...) to OUTFILE, the input of depmap.py and check_links.py.
setlocal
if "%~2"=="" ( echo usage: symbols.bat OBJDIR OUTFILE & exit /b 2 )
if not exist "%~1\*.obj" ( echo symbols.bat: no objects in "%~1" & exit /b 1 )
call "%~dp0..\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
dumpbin /nologo /symbols "%~1\*.obj" > "%~2"
if errorlevel 1 ( echo symbols.bat: dumpbin failed & exit /b 1 )
exit /b 0
