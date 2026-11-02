@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "REFERENCE="
if not exist "%ROOT%\build\tests\effect_factories" mkdir "%ROOT%\build\tests\effect_factories"
pushd "%ROOT%\build\tests\effect_factories" || exit /b 1
cl /nologo /W1 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_effect_factories.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\tree.c" /Fe:test_effect_factories.exe /link /OPT:REF user32.lib winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_effect_factories.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
