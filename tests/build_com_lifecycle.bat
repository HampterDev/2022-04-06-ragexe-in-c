@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%"
if not "%~1"=="" set "SOURCE=%~1"
set "OUT=%ROOT%\build\tests\com_lifecycle"
if not "%~2"=="" set "OUT=%~2"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%OUT%" mkdir "%OUT%"
python "%ROOT%\tests\extract_com_lifecycle.py" "%SOURCE%\src\core\winmain.c" "%SOURCE%\src\core\app_win32.c" "%OUT%\com_lifecycle_functions.inc"
if errorlevel 1 exit /b 1
cl /nologo /std:c11 /W3 /WX /wd4101 /wd4505 /wd4189 /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%OUT%" "%ROOT%\tests\test_com_lifecycle.c" /Fo:"%OUT%\test_com_lifecycle.obj" /Fe:"%OUT%\test_com_lifecycle.exe"
if errorlevel 1 exit /b 1
"%OUT%\test_com_lifecycle.exe"
if not "%errorlevel%"=="0" exit /b 1
