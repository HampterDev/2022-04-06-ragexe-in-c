@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\item_use" mkdir "%ROOT%\build\tests\item_use"
pushd "%ROOT%\build\tests\item_use" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_item_use.c" "%ROOT%\src\game\msg_items.c" "%ROOT%\src\session\session_effect_state.c" "%ROOT%\src\platform\clock_win32.c" /Fe:test_item_use.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_item_use.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
