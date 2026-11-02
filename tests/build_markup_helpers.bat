@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\markup_helpers" mkdir "%ROOT%\build\tests\markup_helpers"
pushd "%ROOT%\build\tests\markup_helpers" || exit /b 1
cl /nologo /W1 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_markup_helpers.c" "%ROOT%\src\ui\chat\navi_text.c" "%ROOT%\src\ui\chat\message_tags.c" "%ROOT%\src\ui\chat\markup_links.c" /Fe:test_markup_helpers.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_markup_helpers.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
