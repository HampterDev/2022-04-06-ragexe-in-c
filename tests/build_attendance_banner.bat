@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\attendance-banner-tests"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%OUT%" mkdir "%OUT%"
python "%ROOT%\tests\extract_attendance_banner.py" "%ROOT%\src\ui\quests\attendance.c" "%OUT%\attendance_banner_functions.inc"
if errorlevel 1 exit /b 1
set "INC=/I"%ROOT%\src" /I"%OUT%" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0""
cl /nologo /std:c11 /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %INC% "%ROOT%\tests\test_attendance_banner.c" /Fo:"%OUT%\test_attendance_banner.obj" /Fe:"%OUT%\test_attendance_banner.exe"
if errorlevel 1 exit /b 1
"%OUT%\test_attendance_banner.exe"
if not "%errorlevel%"=="0" exit /b 1
