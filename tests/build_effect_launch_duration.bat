@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\effect_launch_duration"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_effect_launch_duration.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%OUT%" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_effect_launch_duration.c" "%ROOT%\src\effects\durations.c" /Fe:test_effect_launch_duration.exe
if errorlevel 1 (popd & exit /b 1)
test_effect_launch_duration.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
