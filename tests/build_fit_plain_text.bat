@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\fit_plain_text" mkdir "%ROOT%\build\tests\fit_plain_text"
pushd "%ROOT%\build\tests\fit_plain_text" || exit /b 1
cl /nologo /W1 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_fit_plain_text.c" "%ROOT%\src\std\string.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\core\fatal.c" /Fe:test_fit_plain_text.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_fit_plain_text.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
