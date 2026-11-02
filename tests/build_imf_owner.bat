@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\imf" mkdir "%ROOT%\build\tests\imf"
pushd "%ROOT%\build\tests\imf" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_imf_owner.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\fileio\resource_path.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\actor\layer_order.c" "%ROOT%\src\platform\filesystem_win32.c" /Fe:test_imf_owner.exe /link /OPT:REF kernel32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_imf_owner.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
