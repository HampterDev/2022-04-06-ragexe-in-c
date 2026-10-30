@echo off
REM find_vcvars.bat - set VCVARS to a vcvars32.bat, for build.bat, vendor\build_vendor.bat
REM and tools\datatrace\build.bat (called; sets VCVARS in the caller's environment).
REM
REM   1. VCVARS already set to an existing file           -> kept (the manual override)
REM   2. vswhere.exe (ships with every Visual Studio 2017+ installer and Build Tools)
REM      -> the newest installed edition that has the C++ x86/x64 toolset
REM   3. the Visual Studio 2022 Community default path
REM
REM Exit code 1 (with a message) when no vcvars32.bat can be found.
if defined VCVARS if exist "%VCVARS%" exit /b 0
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSROOT="
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if defined VSROOT if exist "%VSROOT%\VC\Auxiliary\Build\vcvars32.bat" set "VCVARS=%VSROOT%\VC\Auxiliary\Build\vcvars32.bat"
if not defined VCVARS set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
if exist "%VCVARS%" exit /b 0
echo *** Cannot find vcvars32.bat ^(looked for vswhere.exe and "%VCVARS%"^)
echo *** Install "Visual Studio - Desktop development with C++" or set VCVARS to your vcvars32.bat
exit /b 1
