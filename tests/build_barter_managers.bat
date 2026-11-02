@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\barter_managers" mkdir "%ROOT%\build\tests\barter_managers"
pushd "%ROOT%\build\tests\barter_managers" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_barter_managers.c" "%ROOT%\tests\barter_managers_unreached.c" "%ROOT%\src\web\barter_market.c" "%ROOT%\src\web\barter_expanded.c" "%ROOT%\src\contents\barter_item.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\tree.c" /Fe:test_barter_managers.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_barter_managers.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
