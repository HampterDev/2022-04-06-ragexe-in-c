@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\bitmap" mkdir "%ROOT%\build\tests\bitmap"
pushd "%ROOT%\build\tests\bitmap" || exit /b 1
cl /nologo /w /O2 /MD /c /D_CRT_SECURE_NO_WARNINGS /FI"%ROOT%\tests\bitmap_alloc_fixture.h" "%ROOT%\vendor\giflib\dgif_lib.c" "%ROOT%\vendor\giflib\gifalloc.c" "%ROOT%\vendor\giflib\gif_err.c" "%ROOT%\vendor\giflib\openbsd-reallocarray.c"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\libpng-1.6.36" "%ROOT%\tests\test_bitmap_owner.c" "%ROOT%\src\std\string.c" dgif_lib.obj gifalloc.obj gif_err.obj openbsd-reallocarray.obj "%ROOT%\build\vendor\imglibs.lib" kernel32.lib user32.lib "%ROOT%\src\core\cstr.c" /Fe:test_bitmap_owner.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_bitmap_owner.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
