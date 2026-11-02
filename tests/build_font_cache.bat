@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\fonts" mkdir "%ROOT%\build\tests\fonts"
pushd "%ROOT%\build\tests\fonts" || exit /b 1
cl /nologo /c /W3 /WX /TC /std:c11 /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" /FI"%ROOT%\tests\font_cache_fixture.h" "%ROOT%\src\render\fonts.c" /Fo:font_cache_test.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /std:c11 /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_font_cache.c" font_cache_test.obj /Fe:test_font_cache.exe /link kernel32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_font_cache.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /c /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\src\render\fonts.c" "%ROOT%\src\core\text_encoding.c" "%ROOT%\tests\test_font_cache_system.c"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /EHsc /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\vendor\font_backend\font_family.cpp" fonts.obj text_encoding.obj test_font_cache_system.obj /Fe:test_font_cache_system.exe /link gdi32.lib user32.lib gdiplus.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_font_cache_system.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
