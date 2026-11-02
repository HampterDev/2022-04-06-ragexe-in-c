@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\language" mkdir "%ROOT%\build\tests\language"
pushd "%ROOT%\build\tests\language" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_language_input.c" "%ROOT%\src\platform\code_page.c" /Fe:test_language_input.exe /link /OPT:REF user32.lib imm32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_language_input.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
