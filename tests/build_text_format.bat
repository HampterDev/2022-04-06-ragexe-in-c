@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\format" mkdir "%ROOT%\build\tests\format"
pushd "%ROOT%\build\tests\format" || exit /b 1
cl /nologo /W4 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_text_format.c" "%ROOT%\src\core\text_markup.c" "%ROOT%\src\core\hex.c" /Fe:test_text_format.exe /link user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_text_format.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_book_reading.c" "%ROOT%\src\core\hex.c" "%ROOT%\src\core\text_array.c" /Fe:test_book_reading.exe /link user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_book_reading.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
