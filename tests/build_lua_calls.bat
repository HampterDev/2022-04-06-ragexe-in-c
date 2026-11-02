@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OUT=%ROOT%\build\tests\lua_calls"
if not exist "%OUT%\runtime" mkdir "%OUT%\runtime"
pushd "%ROOT%\vendor" || exit /b 1
cl /nologo /c /O2 /MD /MP4 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS lua-5.1.4\src\*.c /Fo"%OUT%\runtime\\"
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
type nul > "%OUT%\runtime\objects.rsp"
for %%s in (lua-5.1.4\src\*.c) do >> "%OUT%\runtime\objects.rsp" echo "%OUT%\runtime\%%~ns.obj"
lib /nologo /OUT:"%OUT%\lua_test.lib" @"%OUT%\runtime\objects.rsp"
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
popd
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_lua_calls.c" lua_test.lib "%ROOT%\src\platform\window.c" /Fe:test_lua_calls.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
test_lua_calls.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_lua_calls.c" lua_test.lib /Fe:test_lua_calls_disabled.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
test_lua_calls_disabled.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_lua_boundaries.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" lua_test.lib "%ROOT%\src\platform\window.c" /Fe:test_lua_boundaries.exe /link /OPT:REF user32.lib
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
test_lua_boundaries.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
popd
exit /b 0
