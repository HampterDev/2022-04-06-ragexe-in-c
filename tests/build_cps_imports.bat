@echo off
setlocal
set "ROOT=%~dp0.."
set "EXE=%ROOT%\build\ragexe.exe"
if not "%~1"=="" set "EXE=%~1"
if not exist "%EXE%" (
    echo cps imports: %EXE% not found; run build.bat first.
    exit /b 1
)
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
dumpbin /nologo /imports:cps.dll "%EXE%" | findstr /r /c:"^  *[0-9A-F][0-9A-F]* uncompress$" >nul
if errorlevel 1 (
    echo cps imports: uncompress is not imported from cps.dll
    exit /b 1
)
dumpbin /nologo /imports:cps.dll "%EXE%" | findstr /r /c:"^  *[0-9A-F][0-9A-F]* compress$" >nul
if errorlevel 1 (
    echo cps imports: compress is not imported from cps.dll
    exit /b 1
)
echo cps imports: compress and uncompress come from cps.dll, as in the original, passed.
exit /b 0
