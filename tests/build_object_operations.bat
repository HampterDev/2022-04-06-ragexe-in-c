@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\object_operations" mkdir "%ROOT%\build\tests\object_operations"
pushd "%ROOT%\build\tests\object_operations" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_object_operations.c" "%ROOT%\src\actor\object_ops.c" "%ROOT%\src\effects\skills\dummy_effect_launcher.c" "%ROOT%\src\world\objects.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\core\abstract_method.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\tree.c" /Fe:test_object_operations.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_object_operations.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
