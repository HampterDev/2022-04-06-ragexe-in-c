@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\login_pin_state" mkdir "%ROOT%\build\tests\login_pin_state"
pushd "%ROOT%\build\tests\login_pin_state" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_login_pin_state.c" "%ROOT%\src\login\login_dispatch.c" "%ROOT%\src\login\char_results.c" ws2_32.lib /Fe:test_login_pin_state.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_login_pin_state.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
