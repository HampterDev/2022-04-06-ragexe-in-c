@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\render_operations" mkdir "%ROOT%\build\tests\render_operations"
pushd "%ROOT%\build\tests\render_operations" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /DGRANNY_IMPORT= /D_CRT_SECURE_NO_WARNINGS /FI"%ROOT%\tests\render_operations_boundaries.h" /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_render_operations.c" "%ROOT%\src\actor\sprite_overlays.c" "%ROOT%\src\actor\motion_update.c" /Fe:test_render_operations.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_render_operations.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
