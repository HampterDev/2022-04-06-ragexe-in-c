@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\inventory_match" mkdir "%ROOT%\build\tests\inventory_match"
pushd "%ROOT%\build\tests\inventory_match" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_inventory_match.c" "%ROOT%\src\session\inventory_match.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" /Fe:test_inventory_match.exe /link /OPT:REF user32.lib advapi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_inventory_match.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
