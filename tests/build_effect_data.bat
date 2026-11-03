@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\vendor\yyjson.lib" call "%ROOT%\vendor\build_vendor.bat"
if errorlevel 1 exit /b 1
if not exist "%ROOT%\build\tests\effect_data" mkdir "%ROOT%\build\tests\effect_data"
pushd "%ROOT%\build\tests\effect_data" || exit /b 1
rem The effect data as build.bat embeds it.
cl /nologo /O2 /MD "%ROOT%\tools\bin2c.c" /Fe:bin2c.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
bin2c.exe effect_data_embedded.c g_effectData "%ROOT%\data\effects\effects.json"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /std:c11 /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_effect_data.c" "%ROOT%\src\fileio\resource_path.c" effect_data_embedded.c /Fe:test_effect_data.exe /link "%ROOT%\build\vendor\yyjson.lib"
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_effect_data.exe "%CD%"
set "CODE=%errorlevel%"
popd
exit /b %CODE%
