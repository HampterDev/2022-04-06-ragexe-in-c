@echo off
REM tests\oracle\build.bat - the oracle tests: the original executable mapped into the test process
REM and its pure routines called against the lifted C over their whole input space.
REM The original is %ORACLE_EXE%, by default ..\client-files\2022-04-06_Ragexe_1648707856.exe beside
REM this checkout. Every test is a 32-bit program linked away from the original's base 0x400000.
setlocal
set "ROOT=%~dp0..\.."
set "OUT=%ROOT%\build\tests\oracle"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%OUT%" mkdir "%OUT%"
if "%ORACLE_EXE%"=="" set "ORACLE_EXE=%ROOT%\..\client-files\2022-04-06_Ragexe_1648707856.exe"
if not exist "%ORACLE_EXE%" (echo oracle: original executable not found at "%ORACLE_EXE%"; set ORACLE_EXE & exit /b 1)
set "CFLAGS=/nologo /W3 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\tests\oracle" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include""
set "LFLAGS=/link /BASE:0x10000000 /FIXED /DYNAMICBASE:NO user32.lib"
pushd "%OUT%" || exit /b 1

cl %CFLAGS% "%ROOT%\tests\oracle\test_skill_poses.c" "%ROOT%\tests\oracle\oracle_image.c" "%ROOT%\src\session\session_use_effect.c" "%ROOT%\src\session\session_cast_effect.c" /Fe:test_skill_poses.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_skill_poses.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

popd
exit /b 0
