@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "REFERENCE="
if not exist "%ROOT%\build\tests\skill_effect_calls" mkdir "%ROOT%\build\tests\skill_effect_calls"
pushd "%ROOT%\build\tests\skill_effect_calls" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_skill_effect_calls.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" /Fe:test_skill_effect_calls.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_skill_effect_calls.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
