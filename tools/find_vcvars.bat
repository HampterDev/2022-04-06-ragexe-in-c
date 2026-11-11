@echo off
REM find_vcvars.bat [x86] - set VCVARS to a vcvars64.bat, for build.bat, vendor\build_vendor.bat,
REM the tests and the tools (called; sets VCVARS in the caller's environment). With x86 it is the
REM vcvars32.bat beside it instead, for the two scripts that build 32-bit programs: tests\oracle,
REM which loads the original executable, and tools\granny_export, which loads the game's granny2.dll.
REM
REM   1. VCVARS already set to an existing file           -> kept (the manual override; either
REM      vcvars64.bat or vcvars32.bat, the other one is taken from beside it)
REM   2. vswhere.exe (ships with every Visual Studio 2017+ installer and Build Tools)
REM      -> the newest installed edition that has the C++ x86/x64 toolset
REM   3. the Visual Studio 2022 Community default path
REM
REM Exit code 1 (with a message) when no such vcvars file can be found.
if defined VCVARS if exist "%VCVARS%" goto width
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSROOT="
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if defined VSROOT if exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
:width
if /i "%~1"=="x86" (set "VCVARS=%VCVARS:vcvars64.bat=vcvars32.bat%") else (set "VCVARS=%VCVARS:vcvars32.bat=vcvars64.bat%")
if exist "%VCVARS%" exit /b 0
echo *** Cannot find "%VCVARS%" ^(looked for vswhere.exe^)
echo *** Install "Visual Studio - Desktop development with C++" or set VCVARS to your vcvars64.bat
exit /b 1
