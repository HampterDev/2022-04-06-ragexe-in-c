@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\vending_retry" mkdir "%ROOT%\build\tests\vending_retry"
pushd "%ROOT%\build\tests\vending_retry" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_vending_retry.c" "%ROOT%\src\game\msg_vending.c" "%ROOT%\src\session\session_items.c" "%ROOT%\src\session\session_item_lists.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\list.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\core\fatal.c" "%ROOT%\src\iteminfo\item_list.c" /Fe:test_vending_retry.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_vending_retry.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
