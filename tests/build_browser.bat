@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests" mkdir "%ROOT%\build\tests"
pushd "%ROOT%\build\tests" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_browser.c" /Fe:test_browser.exe /link /OPT:REF user32.lib oleaut32.lib ole32.lib uuid.lib
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
test_browser.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_browser_owners.c" "%ROOT%\tests\browser_owner_support.c" "%ROOT%\src\ui\wndmgr\teardown.c" "%ROOT%\src\ui\wndmgr\wndmgr_std.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\tree.c" /Fe:test_browser_owners.exe /link /OPT:REF user32.lib
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
test_browser_owners.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_browser_com.c" "%ROOT%\src\browser\browser_address.c" /Fe:test_browser_com.exe /link /OPT:REF user32.lib oleaut32.lib ole32.lib uuid.lib
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
test_browser_com.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
popd
exit /b 0
