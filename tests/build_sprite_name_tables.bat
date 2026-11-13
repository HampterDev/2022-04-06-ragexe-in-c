@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\sprite_name_tables" mkdir "%ROOT%\build\tests\sprite_name_tables"
pushd "%ROOT%\build\tests\sprite_name_tables" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_sprite_name_tables.c" "%ROOT%\src\session\session_std.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" /Fe:test_sprite_name_tables.exe /link /OPT:REF winmm.lib user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_sprite_name_tables.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
