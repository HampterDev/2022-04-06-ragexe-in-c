@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\text_render" mkdir "%ROOT%\build\tests\text_render"
pushd "%ROOT%\build\tests\text_render" || exit /b 1
cl /nologo /W3 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_text_render.c" "%ROOT%\src\platform\charset.c" "%ROOT%\src\core\text_format.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\core\hex.c" /Fe:test_text_render.exe /link user32.lib gdi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_text_render.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_edit_text_render.c" /Fe:test_edit_text_render.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_edit_text_render.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
